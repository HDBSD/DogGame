param
(
    [Parameter(Mandatory = $true)]
    [int]$ProcessId,

    [ValidateRange(5, 60)]
    [int]$Seconds = 20,

    [Parameter(Mandatory = $true)]
    [string]$OutputDirectory
)

$ErrorActionPreference = 'Stop'
$targetProcess = Get-Process -Id $ProcessId -ErrorAction Stop
$targetExecutable = $targetProcess.Path
$recorder = (Get-Command wpr.exe -ErrorAction Stop).Source
$outputRoot = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $outputRoot -Force | Out-Null
$tracePath = Join-Path $outputRoot 'cpu.etl'

if (Test-Path -LiteralPath $tracePath)
{
    throw "A capture already exists at $tracePath. Choose a new output directory."
}

# WPR owns a machine-wide session. Never cancel or replace another recording.
# A failed start leaves that session alone; cleanup stops only our successful
# start. Capture includes all processes, but the target ID supports filtering.
$recordingStarted = $false
$captureStart = [DateTime]::UtcNow
try
{
    & $recorder -start CPU -filemode
    if ($LASTEXITCODE -ne 0)
    {
        throw 'Windows CPU recording could not start. Administrator rights may be required, or another recording may be active.'
    }
    $recordingStarted = $true
    Write-Host "Recording CPU samples and thread scheduling for PID $ProcessId for $Seconds seconds."
    $exitedDuringCapture = $targetProcess.WaitForExit($Seconds * 1000)
}
finally
{
    if ($recordingStarted)
    {
        & $recorder -stop $tracePath
        if ($LASTEXITCODE -ne 0)
        {
            throw 'Windows could not save the CPU recording. Inspect wpr -status before starting another capture.'
        }
    }
}

[ordered]@{
    ProcessId = $ProcessId
    Executable = $targetExecutable
    CaptureStartUtc = $captureStart.ToString('o')
    CaptureEndUtc = [DateTime]::UtcNow.ToString('o')
    ProcessExited = $exitedDuringCapture
    Trace = $tracePath
    Scope = 'System CPU sampling and scheduling; filter analysis by ProcessId. No game mutations or process termination.'
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $outputRoot 'capture.json') -Encoding UTF8

Write-Host "Saved $tracePath"
