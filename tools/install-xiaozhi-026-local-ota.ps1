# BOTIZIN S3: upload verified XiaoZhi 0.0.26 via notebook LAN, log COM4.
# Windows PowerShell 5.1+; leave CORE 0.0.25 in ota_0, upload only ota_1.
# No USB flashing or repartition. Only reboot after OTA_WRITE_VERIFIED.
$ErrorActionPreference = 'Stop'
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
Add-Type -AssemblyName System.Net.Http
$base = 'http://192.168.0.18'
$url = 'https://raw.githubusercontent.com/Fernandodsrm/botizin-core-v2/main/releases/0.0.26/firmware.bin'
$expectedBytes = 1942976
$expectedSha = 'c4a83a5a41de7b84c9afd088c0d81decec2563f406506d88ec2873cfe7f658c4'
$dir = Join-Path $env:USERPROFILE 'Downloads'
$bin = Join-Path $dir 'botizin-xiaozhi-0.0.26-verified.bin'
$log = Join-Path $dir ('botizin-install-026-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '.txt')
$writer = $null
$serial = $null
$client = $null
$form = $null
$pending = ''
function Flat([string]$str) { return $str.Replace([string][char]13,' ').Replace([string][char]10,' | ') }
function Note([string]$str) {
    $line = (Get-Date -Format 'HH:mm:ss.fff') + ' ' + $str
    Write-Host $line
    if ($null -ne $script:writer) { $script:writer.WriteLine($line) }
}
function Read-Serial {
    if ($null -eq $script:serial -or !$script:serial.IsOpen) { return }
    try {
        $script:pending += $script:serial.ReadExisting()
        while ($script:pending.Contains([char]10)) {
            $n = $script:pending.IndexOf([char]10)
            $line = $script:pending.Substring(0,$n).TrimEnd([char]13)
            $script:pending = $script:pending.Substring($n+1)
            if ($line -match '(?i)PASSWORD|WIFI_SETUP_SSID_USB_ONLY|TOKEN|API_KEY|SECRET|authorization:') { $line = '[REDACTED_CREDENTIAL_LINE]' }
            $script:writer.WriteLine((Get-Date -Format 'HH:mm:ss.fff') + ' SERIAL ' + $line)
            if ($line -match '(?i)DOWNLOAD|OTA_|UPDATE|SHA_|RESET_REASON|INT_WDT|TASK_WDT|GURU MEDITATION|PANIC|FAIL|BOOT_PARTITION|REBOOT|[Ee]rror') { Write-Host ('SERIAL: ' + $line) }
        }
        if ($script:pending.Length -gt 8192) { $script:pending = '' }
    } catch { Note ('SERIAL_READ_ERROR: ' + $_.Exception.Message) }
}
function Await-Monitor($task,[int]$seconds) {
    $started = Get-Date
    while (!$task.IsCompleted -and ((Get-Date)-$started).TotalSeconds -lt $seconds) {
        Read-Serial
        Start-Sleep -Milliseconds 100
    }
    Read-Serial
    if (!$task.IsCompleted) { throw 'HTTP TIMEOUT; no reboot requested' }
    return $task.GetAwaiter().GetResult()
}
try {
    if (!(Test-Path $dir)) { New-Item -Path $dir -ItemType Directory | Out-Null }
    $writer = [System.IO.StreamWriter]::new($log,$false,[System.Text.UTF8Encoding]::new($false))
    $writer.AutoFlush = $true
    Note 'INICIO: OTA LOCAL; log salvo em Downloads. O firmware sera baixado no notebook.'
    $serial = [System.IO.Ports.SerialPort]::new('COM4',115200,[System.IO.Ports.Parity]::None,8,[System.IO.Ports.StopBits]::One)
    $serial.DtrEnable = $false
    $serial.RtsEnable = $false
    $serial.Handshake = [System.IO.Ports.Handshake]::None
    try { $serial.Open() } catch { throw ('COM4 ocupada: feche o Monitor Serial no Arduino IDE. ' + $_.Exception.Message) }
    Note 'COM4 aberta para registro. DTR e RTS desligados.'
    $handler = [System.Net.Http.HttpClientHandler]::new()
    $handler.UseProxy = $false
    $client = [System.Net.Http.HttpClient]::new($handler)
    $client.Timeout = [TimeSpan]::FromSeconds(240)
    $status = Await-Monitor ($client.GetStringAsync($base + '/status')) 15
    Note ('ANTES: ' + (Flat $status))
    if ($status -notmatch 'BOTIZIN CORE V0[.]0[.]25') { throw 'STOP: firmware na placa nao e CORE 0.0.25' }
    if ($status -notmatch 'RUNNING_PARTITION:\s*app0\s*/\s*ota_0') { throw 'STOP: CORE nao esta no app0/ota_0' }
    if ($status -notmatch 'NEXT_UPDATE_PARTITION:\s*app1\s*/\s*ota_1') { throw 'STOP: slot livre nao e app1/ota_1' }
    if ($status -notmatch 'OTA:\s*READY') { throw 'STOP: OTA nao esta READY' }
    Note 'PREFLIGHT OK: app0 fica preservada, destino app1.'
    Invoke-WebRequest -Uri $url -OutFile $bin -UseBasicParsing
    $length = (Get-Item $bin).Length
    $sha = (Get-FileHash -Algorithm SHA256 $bin).Hash.ToLowerInvariant()
    $stream = [System.IO.File]::OpenRead($bin)
    try { $magic = $stream.ReadByte() } finally { $stream.Close() }
    Note ('ARQUIVO: bytes=' + $length + ' sha256=' + $sha + ' header=' + $magic)
    if ($length -ne $expectedBytes -or $sha -ne $expectedSha -or $magic -ne 233) { throw 'STOP: arquivo diferente do binario ESP32 validado; nada gravado' }
    $file = [System.IO.File]::OpenRead($bin)
    $form = [System.Net.Http.MultipartFormDataContent]::new()
    $part = [System.Net.Http.StreamContent]::new($file,4096)
    $part.Headers.ContentType = [System.Net.Http.Headers.MediaTypeHeaderValue]::Parse('application/octet-stream')
    $form.Add($part,'firmware','firmware.bin')
    $client.DefaultRequestHeaders.ExpectContinue = $false
    Note 'ENVIANDO 0.0.26 pelo Wi-Fi local. Monitorando serial e integridade.'
    $target = $base + '/update?size=' + $expectedBytes + '&sha256=' + $expectedSha
    $reply = Await-Monitor ($client.PostAsync($target,$form)) 210
    $body = Await-Monitor ($reply.Content.ReadAsStringAsync()) 15
    Note ('UPLOAD HTTP ' + [int]$reply.StatusCode + ': ' + (Flat $body))
    if (!$reply.IsSuccessStatusCode -or $body -notmatch 'OTA_WRITE_VERIFIED:\s*YES') { throw 'STOP: OTA nao verificada, sem reboot. Consulte o log.' }
    Note 'GRAVACAO CONFERIDA: OTA_WRITE_VERIFIED YES. Solicitando reboot.'
    $empty = [System.Net.Http.StringContent]::new('')
    $reboot = Await-Monitor ($client.PostAsync($base + '/reboot',$empty)) 15
    Note ('REBOOT HTTP ' + [int]$reboot.StatusCode)
    if (!$reboot.IsSuccessStatusCode) { throw 'STOP: nao reiniciou; verifique status antes de outra tentativa' }
    Note 'Aguardando novo boot e pagina /botizin.'
    $ok = $false
    $started = Get-Date
    while (((Get-Date)-$started).TotalSeconds -lt 70) {
        Read-Serial
        Start-Sleep -Milliseconds 400
        if (((Get-Date)-$started).TotalSeconds -lt 7) { continue }
        try {
            $res = Await-Monitor ($client.GetAsync($base + '/botizin')) 4
            $html = Await-Monitor ($res.Content.ReadAsStringAsync()) 4
            if ([int]$res.StatusCode -eq 200 -and $html.Length -gt 40 -and $html -notmatch '(?i)not found: /botizin') {
                Note ('XIAOZHI_PAGINA_RESPONDE: HTTP 200, bytes=' + $html.Length)
                $ok = $true
                break
            }
        } catch {}
    }
    if ($ok) { Note 'RESULTADO: XiaoZhi respondeu por HTTP; audio/IA ainda exigem teste funcional.' }
    else { Note 'RESULTADO: boot /botizin nao confirmado. Verifique log antes de qualquer outra gravacao.' }
} catch {
    Note ('ERRO / ABORTADO COM SEGURANCA: ' + $_.Exception.Message)
    try {
        if ($null -ne $client) {
            $endStatus = Await-Monitor ($client.GetStringAsync($base + '/status')) 6
            Note ('STATUS APOS ERRO: ' + (Flat $endStatus))
        }
    } catch { Note 'Status indisponivel apos erro; consulte as ultimas linhas SERIAL.' }
} finally {
    if ($null -ne $serial) {
        Read-Serial
        if ($serial.IsOpen) { $serial.Close() }
        $serial.Dispose()
    }
    if ($null -ne $form) { $form.Dispose() }
    if ($null -ne $client) { $client.Dispose() }
    if ($null -ne $writer) { $writer.Flush(); $writer.Close() }
    Write-Host ('LOG: ' + $log) -ForegroundColor Cyan
    Write-Host 'Se falhar, envie este unico arquivo de log na conversa.' -ForegroundColor Cyan
}
