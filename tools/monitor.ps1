<#
.SYNOPSIS
    Print USART output from the Nucleo-H563ZI ST-Link Virtual COM Port.

.PARAMETER Port
    COM port name (e.g. COM3). Auto-detected from the ST-Link VCP if omitted.

.PARAMETER Baud
    Baud rate. Defaults to 115200 (matches the firmware).

.EXAMPLE
    tools\monitor.ps1
    tools\monitor.ps1 -Port COM3
#>
param(
    [string]$Port,
    [int]$Baud = 115200
)

if (-not $Port) {
    # Win32_SerialPort misses USB CDC ports, so match the PnP device name instead.
    $dev = Get-CimInstance Win32_PnPEntity |
        Where-Object { $_.Name -match 'Virtual COM' -and $_.Name -match '\(COM\d+\)' } |
        Select-Object -First 1
    if ($dev -and $dev.Name -match '\((COM\d+)\)') {
        $Port = $Matches[1]
        Write-Host "Auto-detected $Port ($($dev.Name))" -ForegroundColor DarkGray
    }
    else {
        Write-Error "No ST-Link Virtual COM Port found. Pass -Port COMx explicitly."
        exit 1
    }
}

$sp = [System.IO.Ports.SerialPort]::new($Port, $Baud, 'None', 8, 'One')
$sp.ReadTimeout = 500
$sp.NewLine = "`n"

try {
    $sp.Open()
}
catch {
    Write-Error "Could not open ${Port}: $($_.Exception.Message)"
    Write-Host "Something else owns the port. Close STM32CubeIDE, STM32CubeProgrammer," -ForegroundColor Yellow
    Write-Host "PuTTY or Tera Term, then run this again." -ForegroundColor Yellow
    exit 1
}

Write-Host "Listening on $Port at $Baud 8N1. Press the black NRST button on the board." -ForegroundColor Cyan
Write-Host "Ctrl+C to stop.`n" -ForegroundColor DarkGray

try {
    while ($true) {
        try { $sp.ReadLine() }
        catch [TimeoutException] { }
    }
}
finally {
    $sp.Close()
    $sp.Dispose()
    Write-Host "`nClosed $Port." -ForegroundColor Cyan
}
