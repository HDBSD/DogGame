[CmdletBinding()]
param(
    [string]$GameDirectory,
    [string]$XboxRecompDirectory,
    [switch]$PreviewBuildLocking,
    [switch]$SkipGeneration,
    [Parameter(Mandatory = $true)][ValidateNotNullOrEmpty()][string]$ToolchainDirectory,
    [string]$BuildDirectory
)
$ErrorActionPreference = 'Stop'

function Invoke-Checked {
    param([string]$Program, [string[]]$Arguments)
    # Python's pipeline writes progress to stderr; Windows PowerShell must not
    # turn successful progress messages into terminating NativeCommandError.
    $ErrorActionPreference = 'Continue'
    & $Program @Arguments 2>&1 | ForEach-Object { Write-Host $_ }
    if ($LASTEXITCODE -ne 0) { throw "$Program failed (exit $LASTEXITCODE)." }
}

# Build tools are always taken from this explicit portable directory.
$tools = (Resolve-Path -LiteralPath $ToolchainDirectory).Path
$python = Join-Path $tools 'python\python.exe'
$git = Join-Path $tools 'git\cmd\git.exe'
$cmake = Join-Path $tools 'cmake\bin\cmake.exe'
$compiler = (Join-Path $tools 'llvm\bin').Replace('\', '/')
$env:PATH = "$compiler;$tools\python;$tools\cmake\bin;$tools\ninja;$tools\git\cmd;$env:SystemRoot\System32;$env:SystemRoot"
foreach ($tool in @($python, $git, $cmake, "$compiler/x86_64-w64-mingw32-clang.exe", "$tools\ninja\ninja.exe"))
{
    if (-not (Test-Path -LiteralPath $tool -PathType Leaf)) { throw "Missing portable tool: $tool" }
}
Invoke-Checked $python @('-c', 'import capstone; print(capstone.__version__)')

if (-not $GameDirectory)
{
    $GameDirectory = Read-Host 'Path to the extracted Xbox game folder containing default.xbe'
}

$game = (Resolve-Path -LiteralPath $GameDirectory).Path
$xbe = Join-Path $game 'default.xbe'

if (-not (Test-Path -LiteralPath $xbe -PathType Leaf))
{
    throw 'The selected folder does not contain default.xbe.'
}

$hash = (Get-FileHash -LiteralPath $xbe -Algorithm SHA256).Hash

if ($hash -notin @('E593C49BB9B403ACC7CC4416B83D6130322DF1B70B89E17038FD437044F37459', 'E6947834C4F9AE096B720D8B123EB8F27E69077E356EDEF327DCA30D43674814'))
{
    throw 'This XBE revision is not supported by the current DogGame configuration.'
}

$build = if ($BuildDirectory) { [IO.Path]::GetFullPath($BuildDirectory) } else { Join-Path $PSScriptRoot 'build' }
$release = Join-Path $build 'Release'
$exe = Join-Path $release 'DogGame.exe'

foreach ($running in @(Get-Process DogGame -ErrorAction SilentlyContinue))
{
    if ($running.Path -eq $exe)
    {
        throw 'Close DogGame.exe before building. Setup will not stop your game.'
    }
}

$lock = Get-Content (Join-Path $PSScriptRoot 'config\xboxrecomp.lock.json') -Raw | ConvertFrom-Json

if (-not $XboxRecompDirectory)
{
    $XboxRecompDirectory = Join-Path $PSScriptRoot '.dependencies\xboxrecomp'

    if (-not (Test-Path -LiteralPath $XboxRecompDirectory))
    {
        Invoke-Checked $git @('clone', '--no-checkout', $lock.repository, $XboxRecompDirectory)
        Invoke-Checked $git @('-C', $XboxRecompDirectory, 'checkout', '--detach', $lock.revision)
    }
}

$XboxRecompDirectory = (Resolve-Path -LiteralPath $XboxRecompDirectory).Path

Invoke-Checked $python @((Join-Path $PSScriptRoot 'scripts\prepare_runtime.py'), '--upstream', $XboxRecompDirectory)

$destination = Join-Path $release 'game'
New-Item -ItemType Directory -Path $destination -Force | Out-Null

if ([IO.Path]::GetFullPath($game).TrimEnd('\') -ne [IO.Path]::GetFullPath($destination).TrimEnd('\'))
{
    # Copy only; never change the user's source game files.
    Get-ChildItem -LiteralPath $game -Force | Copy-Item -Destination $destination -Recurse -Force
}

if ($hash -eq 'E6947834C4F9AE096B720D8B123EB8F27E69077E356EDEF327DCA30D43674814')
{
    # Preserve the existing build's certificate adjustments on the private copy only.
    if ($game -eq $destination)
    {
        throw 'For the original XBE, select a source folder outside build\Release\game.'
    }

    $targetXbe = Join-Path $destination 'default.xbe'
    $bytes = [IO.File]::ReadAllBytes($targetXbe)
    $bytes[0x19c] = 0x20
    $bytes[0x1a4] = 0x20
    $bytes[0x1a6] = 0x2d
    $bytes[0x1b6] = 0x4f
    [IO.File]::WriteAllBytes($targetXbe, $bytes)
}

if (-not $SkipGeneration)
{
    Invoke-Checked $python @((Join-Path $PSScriptRoot 'scripts\generate_game.py'), '--game-dir', $destination)
}
elseif (-not (Test-Path -LiteralPath (Join-Path $PSScriptRoot 'src\recomp\gen\recomp_dispatch.c')))
{
    throw '-SkipGeneration requires previously generated game sources.'
}

$preview = if ($PreviewBuildLocking)
{
    'ON'
}
else
{
    'OFF'
}

$configure = @('-S', $PSScriptRoot, '-B', $build, "-DXBOXRECOMP_DIR=$XboxRecompDirectory", "-DPython3_EXECUTABLE=$python", "-DPREVIEW_BUILD_LOCKING=$preview")
$configure += @('-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release',
    "-DCMAKE_MAKE_PROGRAM=$($tools.Replace('\', '/'))/ninja/ninja.exe",
    "-DCMAKE_C_COMPILER=$compiler/x86_64-w64-mingw32-clang.exe",
    "-DCMAKE_CXX_COMPILER=$compiler/x86_64-w64-mingw32-clang++.exe",
    "-DCMAKE_RC_COMPILER=$compiler/llvm-windres.exe",
    "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=$($release.Replace('\', '/'))")

Invoke-Checked $cmake $configure
Invoke-Checked $cmake @('--build', $build, '--config', 'Release', '--target', 'DogGame', '--parallel', '4')
Write-Host "Built $exe"
