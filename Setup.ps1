[CmdletBinding()]
param(
    [string]$GameDirectory,
    [string]$XboxRecompDirectory,
    [switch]$PreviewBuildLocking,
    [switch]$SkipGeneration
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

# Resolve tools without installing system software or requiring administrator access.
$python = (Get-Command python -ErrorAction SilentlyContinue).Source

if (-not $python)
{
    throw 'Install Python 3.10 or newer and make python available on PATH.' 
}

$git = (Get-Command git -ErrorAction SilentlyContinue).Source

if (-not $git)
{
    throw 'Install Git for Windows first.'
}

$cmake = (Get-Command cmake -ErrorAction SilentlyContinue).Source
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'

if (-not (Test-Path -LiteralPath $vswhere))
{
    throw 'Install Visual Studio with Desktop development with C++ first.'
}

$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath

if (-not $vs) 
{
    throw 'Visual Studio C++ build tools were not found.'
}

if (-not $cmake) 
{
    $cmake = Join-Path $vs 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
}

if (-not (Test-Path -LiteralPath $cmake))
{
    throw 'Install CMake 3.20 or newer.'

}
Invoke-Checked $python @('-c', 'import sys; assert sys.version_info >= (3,10), "Python 3.10+ is required"')

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

$build = Join-Path $PSScriptRoot 'build'
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

# Isolated Python environment; generated artifacts remain local to this project.
$venv = Join-Path $build 'python'

if (-not (Test-Path -LiteralPath (Join-Path $venv 'Scripts\python.exe')))
{
    Invoke-Checked $python @('-m', 'venv', $venv)
}

$python = Join-Path $venv 'Scripts\python.exe'
Invoke-Checked $python @('-m', 'pip', 'install', 'capstone==5.0.6')
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

Invoke-Checked $cmake @('-S', $PSScriptRoot, '-B', $build, "-DXBOXRECOMP_DIR=$XboxRecompDirectory", "-DPython3_EXECUTABLE=$python", "-DPREVIEW_BUILD_LOCKING=$preview")
Invoke-Checked $cmake @('--build', $build, '--config', 'Release', '--target', 'DogGame', '--parallel', '4')
Write-Host "Built $exe"
Write-Host 'Run .\Run-Menu.ps1 to start the game.'
