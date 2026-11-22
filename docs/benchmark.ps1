param(
    [Parameter(Mandatory = $true)][string]$Exe,
    [int]$N = 0,
    [ValidateSet('on', 'off')][string]$Instancing = 'off',
    [int]$WarmupSeconds = 3,
    [int]$Samples = 5,
    [int]$IntervalMs = 500
)

$ErrorActionPreference = 'Stop'

$arguments = @('--instancing', $Instancing)
if ($N -gt 0) {
    $arguments = @('--n', $N) + $arguments
}

$process = Start-Process -FilePath $Exe -ArgumentList $arguments -PassThru
try {
    Start-Sleep -Seconds $WarmupSeconds

    for ($i = 0; $i -lt $Samples; $i++) {
        Start-Sleep -Milliseconds $IntervalMs
        $process.Refresh()

        if ($process.HasExited) {
            "EXITED code=$($process.ExitCode)"
            break
        }

        "SAMPLE $($i + 1): $($process.MainWindowTitle)"
    }
} finally {
    if (-not $process.HasExited) {
        Stop-Process -Id $process.Id -Force
        $process.WaitForExit()
    }
}
