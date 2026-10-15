[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$project = Split-Path $PSScriptRoot -Parent
$downloads = Join-Path $project 'build\portable\downloads'
$payload = Join-Path $project 'build\setup\payload'
New-Item -ItemType Directory -Path $downloads, $payload -Force | Out-Null
$lock = Get-Content (Join-Path $PSScriptRoot 'toolchain.lock.json') -Raw | ConvertFrom-Json
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

foreach ($package in $lock.packages)
{
    $file = Join-Path $downloads ($package.name + '.zip')
    if (-not (Test-Path -LiteralPath $file))
    {
        & curl.exe -sSL --fail --retry 2 --output $file $package.url
        if ($LASTEXITCODE -ne 0) { throw "Download failed: $($package.name)" }
    }
    if ((Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash -ne $package.sha256)
    {
        throw "Toolchain checksum mismatch: $($package.name)"
    }
}

$temporary = Join-Path $payload ("tools.$([Guid]::NewGuid().ToString('N')).zip")
$bundle = [IO.Compression.ZipFile]::Open($temporary, [IO.Compression.ZipArchiveMode]::Create)
function Add-Text([string]$Name, [string]$Value)
{
    $entry = $bundle.CreateEntry($Name)
    $writer = New-Object IO.StreamWriter($entry.Open())
    try { $writer.Write($Value) } finally { $writer.Dispose() }
}
try
{
    foreach ($package in $lock.packages)
    {
        $archive = [IO.Compression.ZipFile]::OpenRead((Join-Path $downloads ($package.name + '.zip')))
        try
        {
            foreach ($entry in $archive.Entries)
            {
                if ($entry.FullName.EndsWith('/')) { continue }
                $relative = $entry.FullName
                if ($package.name -in @('llvm', 'cmake'))
                {
                    $relative = $relative.Substring($relative.IndexOf('/') + 1)
                }
                $folder = if ($package.name -eq 'capstone') { 'python/Lib/site-packages' } else { $package.name }
                $target = $bundle.CreateEntry("$folder/$relative", [IO.Compression.CompressionLevel]::Optimal)
                $inputStream = $entry.Open()
                $outputStream = $target.Open()
                try { $inputStream.CopyTo($outputStream) }
                finally { $outputStream.Dispose(); $inputStream.Dispose() }
            }
        }
        finally { $archive.Dispose() }
    }
    # Isolated Python deliberately uses our bundled dependencies, without pip/venv.
    # Add the working directory for xboxrecomp's `python -m tools.*` entry points.
    Add-Text 'python/portable._pth.template' "python313.zip`n.`nLib/site-packages`nimport site`n"
    Add-Text 'python/Lib/site-packages/sitecustomize.py' "import os, sys`nsys.path.insert(0, os.getcwd())`n"
    Add-Text 'toolchain.lock.json' (Get-Content (Join-Path $PSScriptRoot 'toolchain.lock.json') -Raw)
    $license = Join-Path $downloads 'ninja-LICENSE.txt'
    if (-not (Test-Path -LiteralPath $license))
    {
        & curl.exe -sSL --fail --output $license 'https://raw.githubusercontent.com/ninja-build/ninja/v1.13.2/COPYING'
        if ($LASTEXITCODE -ne 0) { throw 'Ninja license download failed.' }
    }
    if ((Get-FileHash -LiteralPath $license -Algorithm SHA256).Hash -ne 'EB7E9AB9690124C5C9F42BDC81383D886A3DEDE26345B6ED15BBAD7CAF81F7EA')
    {
        throw 'Ninja license checksum mismatch.'
    }
    Add-Text 'ninja/LICENSE.txt' (Get-Content -LiteralPath $license -Raw)
}
finally { $bundle.Dispose() }
Move-Item -LiteralPath $temporary -Destination (Join-Path $payload 'tools.zip') -Force
(Get-FileHash -LiteralPath (Join-Path $payload 'tools.zip') -Algorithm SHA256).Hash |
    Set-Content -LiteralPath (Join-Path $payload 'tools.sha256')
