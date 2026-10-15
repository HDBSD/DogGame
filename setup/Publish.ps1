[CmdletBinding()]
param([string]$OutputDirectory)
$ErrorActionPreference = 'Stop'
$project = Split-Path $PSScriptRoot -Parent
$output = if ($OutputDirectory) { [IO.Path]::GetFullPath($OutputDirectory) } else { Join-Path $project 'build\setup\publish' }
$env:DOTNET_CLI_HOME = Join-Path $project 'build\setup\dotnet-home'
$env:NUGET_PACKAGES = Join-Path $project 'build\setup\nuget-packages'
$env:DOTNET_CLI_TELEMETRY_OPTOUT = '1'
$env:DOTNET_GENERATE_ASPNET_CERTIFICATE = 'false'
$env:DOTNET_ADD_GLOBAL_TOOLS_TO_PATH = 'false'
& (Join-Path $PSScriptRoot 'Prepare-Toolchain.ps1')
& dotnet publish (Join-Path $PSScriptRoot 'app\DogGame.Setup.csproj') -c Release -r win-x64 --self-contained true `
    -p:PublishSingleFile=true -p:IncludeNativeLibrariesForSelfExtract=true -p:DebugType=embedded -o $output
if ($LASTEXITCODE -ne 0)
{
    throw "Publishing DogGameSetup failed (exit $LASTEXITCODE)."
}
Write-Host "Built $(Join-Path $output 'DogGameSetup.exe')"
