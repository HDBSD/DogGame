[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$ScratchDirectory, [Parameter(Mandatory = $true)][ValidateNotNullOrEmpty()][string]$ToolchainDirectory)
$ErrorActionPreference = 'Stop'
$checks = New-Object 'System.Collections.Generic.List[object]'
function Add-Check([string]$Name, [bool]$Passed, [string]$Detail)
{
    $checks.Add([pscustomobject]@{ Name = $Name; Passed = $Passed; Detail = $Detail })
    $label = if ($Passed) { 'OK' } else { 'MISSING' }
    [Console]::Error.WriteLine("[$label] ${Name}: $Detail")
}
function Read-Tool([string]$Program, [string[]]$Arguments)
{
    if (-not (Test-Path -LiteralPath $Program -PathType Leaf))
    {
        throw "Missing portable tool: $Program"
    }
    $ErrorActionPreference = 'Continue'
    $result = & $Program @Arguments 2>&1 | Out-String
    if ($LASTEXITCODE -ne 0) { throw $result.Trim() }
    return $result.Trim()
}

Add-Check 'Windows x64' ([Environment]::Is64BitOperatingSystem -and [Environment]::Is64BitProcess) 'A 64-bit Windows build process is required.'
$ToolchainDirectory = (Resolve-Path -LiteralPath $ToolchainDirectory).Path
$env:PATH = "$ToolchainDirectory\llvm\bin;$ToolchainDirectory\ninja;$ToolchainDirectory\git\cmd;$ToolchainDirectory\python;$ToolchainDirectory\cmake\bin;$env:SystemRoot\System32;$env:SystemRoot"
$python = Join-Path $ToolchainDirectory 'python/python.exe'
try
{
    if (-not (Test-Path -LiteralPath $python)) { throw 'Bundled Python is missing. Extract the portable toolchain again.' }
    $probe = 'import sys, capstone; assert sys.version_info >= (3,10); print(sys.version.split()[0])'
    $version = Read-Tool -Program $python -Arguments @('-c', $probe)
    if ($version -notmatch '^\d+\.\d+\.\d+') { throw 'Python did not return a usable version. Extract the portable Python/Capstone package again.' }
    Add-Check 'Python build dependencies' $true "$version ($python)"
}
catch { Add-Check 'Python build dependencies' $false $_.Exception.Message }

$git = Join-Path $ToolchainDirectory 'git/cmd/git.exe'
try
{
    if (-not (Test-Path -LiteralPath $git)) { throw 'Bundled Git is missing.' }
    Add-Check 'Git' $true (Read-Tool $git @('--version'))
}
catch { Add-Check 'Git' $false $_.Exception.Message }

$compiler = (Join-Path $ToolchainDirectory 'llvm/bin').Replace('\', '/')
$cmake = Join-Path $ToolchainDirectory 'cmake/bin/cmake.exe'
try
{
    Add-Check 'LLVM-MinGW' $true (Read-Tool "$compiler/x86_64-w64-mingw32-clang.exe" @('--version'))
    Add-Check 'Ninja' $true (Read-Tool (Join-Path $ToolchainDirectory 'ninja/ninja.exe') @('--version'))
}
catch { Add-Check 'Portable compiler tools' $false $_.Exception.Message }
$cmakeReady = $false
try
{
    if (-not $cmake -or -not (Test-Path -LiteralPath $cmake)) { throw 'Bundled CMake 3.20+ is missing.' }
    $description = Read-Tool $cmake @('--version')
    if ($description -notmatch 'cmake version (\d+\.\d+\.\d+)' -or [version]$Matches[1] -lt [version]'3.20.0')
    {
        throw 'CMake 3.20+ is required.'
    }
    Add-Check 'CMake' $true ($description.Split("`n")[0])
    $cmakeReady = $true
}
catch { Add-Check 'CMake' $false $_.Exception.Message }

if ($cmakeReady)
{
    try
    {
        # Compile/link a tiny executable without running it: verify the actual
        # selected C/C++ compiler, Windows SDK, resource compiler and import libraries.
        New-Item -ItemType Directory -Path $ScratchDirectory -Force | Out-Null
        @'
cmake_minimum_required(VERSION 3.20)
project(DogGameRequirements C CXX RC)
add_executable(requirements main.c cpp.cpp resource.rc)
target_link_libraries(requirements PRIVATE d3d11 dxgi dxguid xinput1_4 winmm dbghelp shell32 mfplat mfuuid ole32 winhttp bcrypt)
'@ | Set-Content -LiteralPath (Join-Path $ScratchDirectory 'CMakeLists.txt')
        @'
#include <windows.h>
#include <d3d11.h>
#include <Xinput.h>
#include <mfapi.h>
#include <winhttp.h>
#include <bcrypt.h>
#include <dbghelp.h>
#include <shlobj.h>
extern int check_cpp(void);
static void *volatile imports[] = {(void *)D3D11CreateDevice, (void *)XInputGetState, (void *)MFStartup,
    (void *)WinHttpOpen, (void *)BCryptOpenAlgorithmProvider, (void *)MiniDumpWriteDump, (void *)SHGetFolderPathW};
int main(void) { return imports[0] == 0 || check_cpp() != 1; }
'@ | Set-Content -LiteralPath (Join-Path $ScratchDirectory 'main.c')
        '#include <atomic>' + "`n" + 'extern "C" int check_cpp(void) { std::atomic<int> value{1}; return value.load(); }' |
            Set-Content -LiteralPath (Join-Path $ScratchDirectory 'cpp.cpp')
        '1 RCDATA { 1, 2 }' | Set-Content -LiteralPath (Join-Path $ScratchDirectory 'resource.rc')
        $configure = @('-S', $ScratchDirectory, '-B', (Join-Path $ScratchDirectory 'build'))
        $configure += @('-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release',
            "-DCMAKE_MAKE_PROGRAM=$($ToolchainDirectory.Replace('\', '/'))/ninja/ninja.exe",
            "-DCMAKE_C_COMPILER=$compiler/x86_64-w64-mingw32-clang.exe",
            "-DCMAKE_CXX_COMPILER=$compiler/x86_64-w64-mingw32-clang++.exe",
            "-DCMAKE_RC_COMPILER=$compiler/llvm-windres.exe")

        Read-Tool $cmake $configure | Out-Null
        Read-Tool $cmake @('--build', (Join-Path $ScratchDirectory 'build'), '--config', 'Release') | Out-Null
        Add-Check 'C/C++ compiler, Windows SDK and resources' $true 'The compile/link test passed, including graphics, audio, authentication and resource libraries.'
    }
    catch { Add-Check 'C/C++ compiler, Windows SDK and resources' $false $_.Exception.Message }
}
else
{
    Add-Check 'C/C++ compiler, Windows SDK and resources' $false 'Restore the missing portable tools before the compile test can run.'
}
ConvertTo-Json -InputObject $checks.ToArray() -Depth 4 -Compress
