param(
    [Parameter(Mandatory = $true)][string]$Exe,
    [int]$N = 200,
    [ValidateSet('on', 'off')][string]$Instancing = 'off',
    [Parameter(Mandatory = $true)][string]$Out,
    [int]$WarmupSeconds = 3,
    [double]$FixedTime = -1
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

Add-Type @"
using System;
using System.Runtime.InteropServices;

public static class Dx11CaptureNative
{
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }

    [StructLayout(LayoutKind.Sequential)]
    public struct POINT { public int X, Y; }

    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);

    [DllImport("user32.dll")]
    public static extern bool GetClientRect(IntPtr hWnd, out RECT rect);

    [DllImport("user32.dll")]
    public static extern bool ClientToScreen(IntPtr hWnd, ref POINT point);

    [DllImport("user32.dll")]
    public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdc, uint flags);

    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool ShowWindow(IntPtr hWnd, int command);

    [DllImport("user32.dll")]
    public static extern bool SetWindowPos(IntPtr hWnd, IntPtr insertAfter,
                                           int x, int y, int cx, int cy,
                                           uint flags);
}
"@

$arguments = @('--instancing', $Instancing)
if ($N -gt 0) {
    $arguments = @('--n', $N) + $arguments
}
if ($FixedTime -ge 0) {
    $arguments += @('--fixed-time', $FixedTime)
}

$process = Start-Process -FilePath $Exe -ArgumentList $arguments -PassThru
try {
    Start-Sleep -Seconds $WarmupSeconds
    $process.Refresh()

    if ($process.HasExited) {
        throw "renderer exited with code $($process.ExitCode)"
    }

    $hwnd = $process.MainWindowHandle
    if ($hwnd -eq [IntPtr]::Zero) {
        throw 'renderer has no main window handle'
    }

    [Dx11CaptureNative]::ShowWindow($hwnd, 5) | Out-Null
    [Dx11CaptureNative]::SetWindowPos(
        $hwnd, [IntPtr](-1), 0, 0, 0, 0, 0x0043) | Out-Null
    [Dx11CaptureNative]::SetForegroundWindow($hwnd) | Out-Null
    Start-Sleep -Milliseconds 500

    $rect = New-Object Dx11CaptureNative+RECT
    [Dx11CaptureNative]::GetWindowRect($hwnd, [ref]$rect) | Out-Null
    $width = $rect.Right - $rect.Left
    $height = $rect.Bottom - $rect.Top

    if ($width -le 0 -or $height -le 0) {
        throw "invalid window size ${width}x${height}"
    }

    $bitmap = New-Object System.Drawing.Bitmap($width, $height)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $hdc = $graphics.GetHdc()
    $printed = [Dx11CaptureNative]::PrintWindow($hwnd, $hdc, 2)
    $graphics.ReleaseHdc($hdc)
    $graphics.Dispose()

    if ($printed) {
        $bitmap.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
        "CAPTURE=PrintWindow"
    } else {
        $bitmap.Dispose()
        $bitmap = New-Object System.Drawing.Bitmap($width, $height)
        $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
        $graphics.CopyFromScreen($rect.Left, $rect.Top, 0, 0,
                                 $bitmap.Size)
        $graphics.Dispose()
        $bitmap.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
        "CAPTURE=CopyFromScreen"
    }

    $bitmap.Dispose()
    "SAVED=$Out"
} finally {
    if (-not $process.HasExited) {
        Stop-Process -Id $process.Id -Force
        $process.WaitForExit()
    }
}
