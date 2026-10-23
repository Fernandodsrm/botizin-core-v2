# BOTIZIN S3 OTA serial observer; read-only Windows PowerShell 5.1.
# Run on Windows notebook with S3 connected by USB to COM4. CLOSE Arduino Serial Monitor first.
# Never changes firmware, flash, boot selection, or network settings.
$ErrorActionPreference = "Stop"
$portName = "COM4"
$baud = 115200
$durationMinutes = 15
$folder = Join-Path $env:USERPROFILE "Downloads"
$log = Join-Path $folder ("botizin-ota-com4-" + (Get-Date -Format "yyyyMMdd-HHmmss") + ".txt")
$serial = New-Object System.IO.Ports.SerialPort($portName, $baud, [System.IO.Ports.Parity]::None, 8, [System.IO.Ports.StopBits]::One)
$serial.DtrEnable = $false
$serial.RtsEnable = $false
$serial.Handshake = [System.IO.Ports.Handshake]::None
$serial.ReadTimeout = 500
$serial.Encoding = [System.Text.Encoding]::ASCII
$writer = $null
$pending = ""
function Record-Line([string]$line) {
  $line = $line.TrimEnd([char]13)
  if ([string]::IsNullOrWhiteSpace($line)) { return }
  # Remove credential-bearing lines from local output.
  if ($line -match '(?i)PASSWORD|WIFI_SETUP_SSID_USB_ONLY|TOKEN|API_KEY|SECRET|authorization:') {
    $line = "[REDACTED_CREDENTIAL_LINE]"
  }
  $entry = (Get-Date -Format "HH:mm:ss.fff") + " " + $line
  $script:writer.WriteLine($entry)
  if ($line -match '(?i)INT_WDT|TASK_WDT|GURU MEDITATION|BACKTRACE|PANIC|RESET_REASON|OTA_WRITE_VERIFIED|Update.end|SHA_|DOWNLOAD_PROGRESS|TARGET_VERSION|BOOT_PARTITION|INTERNET_OTA:|EVT[|]|FAIL:') {
    Write-Host $entry
  }
}
try {
  $serial.Open()
  $writer = New-Object System.IO.StreamWriter($log, $false, (New-Object System.Text.UTF8Encoding($false)))
  $writer.AutoFlush = $true
  Write-Host ("MONITORANDO COM4 — " + $durationMinutes + " minutos. LOG: " + $log) -ForegroundColor Cyan
  Write-Host "Somente leitura. Ctrl+C interrompe." -ForegroundColor Green
  $deadline = (Get-Date).AddMinutes($durationMinutes)
  while ((Get-Date) -lt $deadline) {
    $data = $serial.ReadExisting()
    if ($data.Length -gt 0) {
      $pending += $data
      while ($pending.Contains([char]10)) {
        $n = $pending.IndexOf([char]10)
        $line = $pending.Substring(0,$n)
        $pending = $pending.Substring($n+1)
        Record-Line $line
      }
      if ($pending.Length -gt 4096) { Record-Line $pending; $pending = "" }
    }
    Start-Sleep -Milliseconds 120
  }
  if ($pending.Length -gt 0) { Record-Line $pending }
} catch {
  Write-Host ("MONITOR_ERR: " + $_.Exception.Message) -ForegroundColor Red
  Write-Host "Se a COM4 estiver ocupada, feche o Monitor Serial do Arduino IDE e tente novamente."
} finally {
  if ($null -ne $writer) { $writer.Flush(); $writer.Close() }
  if ($serial.IsOpen) { $serial.Close() }
  $serial.Dispose()
  Write-Host ("LOG SALVO: " + $log) -ForegroundColor Cyan
}
