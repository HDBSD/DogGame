[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$ScratchDirectory)
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
    $ErrorActionPreference = 'Continue'
    $result = & $Program @Arguments 2>&1 | Out-String
    if ($LASTEXITCODE -ne 0) { throw $result.Trim() }
    return $result.Trim()
}

Add-Check 'Windows x64' ([Environment]::Is64BitOperatingSystem -and [Environment]::Is64BitProcess) 'A 64-bit Windows build process is required.'
$python = (Get-Command python -ErrorAction SilentlyContinue).Source
try
{
    if (-not $python) { throw 'Install Python 3.10+ and make python available on PATH.' }
    $version = Read-Tool -Program $python -Arguments @('-c', 'import sys, venv, ensurepip; assert sys.version_info >= (3,10); print(sys.version.split()[0])')
    if ($version -notmatch '^\d+\.\d+\.\d+') { throw 'Python did not return a usable version. Install Python 3.10+ with venv and ensurepip support.' }
    Add-Check 'Python and virtual environments' $true "$version ($python)"
}
catch { Add-Check 'Python and virtual environments' $false $_.Exception.Message }

$git = (Get-Command git -ErrorAction SilentlyContinue).Source
try
{
    if (-not $git) { throw 'Install Git for Windows and make git available on PATH.' }
    Add-Check 'Git' $true (Read-Tool $git @('--version'))
}
catch { Add-Check 'Git' $false $_.Exception.Message }

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$visualStudio = $null
try
{
    if (-not (Test-Path -LiteralPath $vswhere)) { throw 'Install Visual Studio or Build Tools with Desktop development with C++.' }
    $visualStudio = Read-Tool $vswhere @('-latest', '-products', '*', '-requires', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath')
    if (-not $visualStudio) { throw 'The Visual Studio x64 C++ toolset was not found.' }
    Add-Check 'Visual Studio C++ tools' $true $visualStudio
}
catch { Add-Check 'Visual Studio C++ tools' $false $_.Exception.Message }

$cmake = (Get-Command cmake -ErrorAction SilentlyContinue).Source
if (-not $cmake -and $visualStudio)
{
    $cmake = Join-Path $visualStudio 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
}
$cmakeReady = $false
try
{
    if (-not $cmake -or -not (Test-Path -LiteralPath $cmake)) { throw 'Install CMake 3.20+ or the Visual Studio CMake tools component.' }
    $description = Read-Tool $cmake @('--version')
    if ($description -notmatch 'cmake version (\d+\.\d+\.\d+)' -or [version]$Matches[1] -lt [version]'3.20.0')
    {
        throw 'CMake 3.20+ is required.'
    }
    Add-Check 'CMake' $true ($description.Split("`n")[0])
    $cmakeReady = $true
}
catch { Add-Check 'CMake' $false $_.Exception.Message }

if ($visualStudio -and $cmakeReady)
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
target_link_libraries(requirements PRIVATE d3d11 dxgi dxguid xinput winmm dbghelp shell32 mfplat mfuuid ole32 winhttp bcrypt)
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
        Read-Tool $cmake @('-S', $ScratchDirectory, '-B', (Join-Path $ScratchDirectory 'build')) | Out-Null
        Read-Tool $cmake @('--build', (Join-Path $ScratchDirectory 'build'), '--config', 'Release') | Out-Null
        Add-Check 'C/C++ compiler, Windows SDK and resources' $true 'The compile/link test passed, including graphics, audio, authentication and resource libraries.'
    }
    catch { Add-Check 'C/C++ compiler, Windows SDK and resources' $false $_.Exception.Message }
}
else
{
    Add-Check 'C/C++ compiler, Windows SDK and resources' $false 'Install the missing Visual Studio/CMake requirements before the compile test can run.'
}
ConvertTo-Json -InputObject $checks.ToArray() -Depth 4 -Compress
