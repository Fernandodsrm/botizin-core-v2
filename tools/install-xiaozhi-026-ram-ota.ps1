# BOTIZIN XiaoZhi 0.0.26 -- OTA local, download in RAM (no disk needed).
# Windows PowerShell 5.1+ / ESP32-S3 CORE 0.0.25 at 192.168.0.18, COM4 optional serial monitor.
# Preserves ota_0, writes ota_1; reboot only when SHA and post-flash verification succeed.
$ErrorActionPreference='Stop'
[Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12
Add-Type -AssemblyName System.Net.Http
$base='http://192.168.0.18'
$fwUrl='https://raw.githubusercontent.com/Fernandodsrm/botizin-core-v2/main/releases/0.0.26/firmware.bin'
$size=1942976
$sha='c4a83a5a41de7b84c9afd088c0d81decec2563f406506d88ec2873cfe7f658c4'
$serial=$null
$client=$null
$payload=$null
$pending=''
$serialLines=0
function Say([string]$v) { Write-Host ((Get-Date -Format 'HH:mm:ss.fff')+' '+$v) }
function Poll-Serial {
    if($null -eq $script:serial -or !$script:serial.IsOpen) { return }
    try {
        $script:pending += $script:serial.ReadExisting()
        while($script:pending.Contains([char]10)){
            $n=$script:pending.IndexOf([char]10)
            $line=$script:pending.Substring(0,$n).TrimEnd([char]13)
            $script:pending=$script:pending.Substring($n+1)
            if($line -match '(?i)PASSWORD|WIFI_SETUP_SSID_USB_ONLY|TOKEN|API_KEY|SECRET|authorization:') {$line='[REDACTED_CREDENTIAL_LINE]'}
            $script:serialLines++
            if($line -match '(?i)OTA|UPDATE|DOWNLOAD|SHA|WRITE|RESET_REASON|INT_WDT|TASK_WDT|GURU MEDITATION|PANIC|FAIL|BOOT_PARTITION|REBOOT|[Ee]rror|HTTP_SERVER') { Say ('COM4: '+$line) }
        }
        if($script:pending.Length -gt 8192) {$script:pending=''}
    } catch { Say ('COM4_READ_ERROR: '+$_.Exception.Message) }
}
function Await-Poll($task,[int]$limit) {
    $start=Get-Date
    while(!$task.IsCompleted -and ((Get-Date)-$start).TotalSeconds -lt $limit){
        Poll-Serial
        Start-Sleep -Milliseconds 100
    }
    Poll-Serial
    if(!$task.IsCompleted) { throw 'Timeout esperando resposta; NENHUM reboot adicional sera solicitado.' }
    return ,($task.GetAwaiter().GetResult())
}
function Flat([string]$s) {return ($s -replace '[\r\n]+',' | ')}
try {
    Say 'BOTIZIN OTA LOCAL EM RAM: nao cria arquivos. COM4 usada apenas para diagnosticos.'
    try {
        $serial=[System.IO.Ports.SerialPort]::new('COM4',115200,[System.IO.Ports.Parity]::None,8,[System.IO.Ports.StopBits]::One)
        $serial.DtrEnable=$false
        $serial.RtsEnable=$false
        $serial.Open()
        Say 'COM4 conectada (DTR/RTS desabilitados).'
    } catch {
        if($null -ne $serial){$serial.Dispose();$serial=$null}
        Say 'AVISO: COM4 ocupada ou indisponivel; OTA local continua, mas sem logs seriais.'
    }
    $handler=[System.Net.Http.HttpClientHandler]::new()
    $handler.UseProxy=$false
    $client=[System.Net.Http.HttpClient]::new($handler)
    $client.Timeout=[TimeSpan]::FromSeconds(240)
    $before=Await-Poll ($client.GetStringAsync($base+'/status')) 15
    Say ('STATUS ANTES: '+(Flat $before))
    if($before -notmatch 'BOTIZIN CORE V0[.]0[.]25'){throw 'PARADO: versao da placa diferente de CORE 0.0.25'}
    if($before -notmatch 'RUNNING_PARTITION:\s*app0\s*/\s*ota_0'){throw 'PARADO: CORE 0.0.25 nao esta no slot ota_0'}
    if($before -notmatch 'NEXT_UPDATE_PARTITION:\s*app1\s*/\s*ota_1'){throw 'PARADO: slot de destino nao e ota_1'}
    if($before -notmatch 'OTA:\s*READY'){throw 'PARADO: OTA da placa nao esta READY'}
    Say 'PREFLIGHT OK: CORE preservado em ota_0; destino ota_1.'
    Say 'Baixando firmware na RAM do notebook, sem gravar em Downloads...'
    [byte[]]$bytes=Await-Poll ($client.GetByteArrayAsync($fwUrl)) 90
    $h=[System.Security.Cryptography.SHA256]::Create()
    try { $digest=([BitConverter]::ToString($h.ComputeHash($bytes))).Replace('-','').ToLowerInvariant() } finally { $h.Dispose() }
    Say ('FIRMWARE RAM bytes='+$bytes.Length+' sha256='+$digest+' magic='+$bytes[0])
    if($bytes.Length -ne $size -or $digest -ne $sha -or $bytes[0] -ne 233){throw 'PARADO: binario diferente do firmware ESP32-S3 validado. Nada gravado.'}
    $payload=[System.Net.Http.MultipartFormDataContent]::new()
    $part=[System.Net.Http.ByteArrayContent]::new($bytes)
    $part.Headers.ContentType=[System.Net.Http.Headers.MediaTypeHeaderValue]::Parse('application/octet-stream')
    $payload.Add($part,'firmware','firmware.bin')
    $client.DefaultRequestHeaders.ExpectContinue=$false
    Say 'ENVIANDO pela rede local para o slot ota_1. Nao feche esta janela.'
    $target=$base+'/update?size='+$size+'&sha256='+$sha
    $reply=Await-Poll ($client.PostAsync($target,$payload)) 215
    $textResult=Await-Poll ($reply.Content.ReadAsStringAsync()) 15
    Say ('HTTP OTA: '+[int]$reply.StatusCode+' '+(Flat $textResult))
    if(!$reply.IsSuccessStatusCode -or $textResult -notmatch 'OTA_WRITE_VERIFIED:\s*YES'){
        throw 'PARADO: nao ha confirmacao de gravacao. NAO reiniciar e enviar as linhas de erro.'
    }
    Say 'SHA e FLASH confirmados pela S3. Solicitando reboot do firmware ja verificado.'
    $empty=[System.Net.Http.ByteArrayContent]::new([byte[]]@())
    $restart=Await-Poll ($client.PostAsync($base+'/reboot',$empty)) 15
    Say ('REBOOT HTTP='+[int]$restart.StatusCode)
    if(!$restart.IsSuccessStatusCode){throw 'OTA gravado mas reboot nao confirmado; nao insistir sem conferir status'}
    Say 'Aguardando inicializacao XiaoZhi em /botizin (primeiro boot).'
    $found=$false
    $started=Get-Date
    while(((Get-Date)-$started).TotalSeconds -lt 80) {
        Poll-Serial
        Start-Sleep -Milliseconds 350
        if(((Get-Date)-$started).TotalSeconds -lt 7){continue}
        try {
            $check=Await-Poll ($client.GetAsync($base+'/botizin')) 4
            $page=Await-Poll ($check.Content.ReadAsStringAsync()) 4
            if([int]$check.StatusCode -eq 200 -and $page.Length -gt 40 -and $page -notmatch '(?i)not found: /botizin'){
                Say ('XIAOZHI /botizin HTTP 200, bytes='+$page.Length)
                $found=$true
                break
            }
        }catch{}
    }
    if($found){Say 'SUCESSO PARCIAL: XiaoZhi responde no browser. Voz e IA precisam de teste funcional.'}
    else {Say 'ATENCAO: /botizin nao respondeu em 80 segundos. Verificar as linhas COM4 antes de repetir.'}
} catch {
    Say ('ERRO: '+$_.Exception.Message)
    try {
        if($null -ne $client){
            $after=Await-Poll ($client.GetStringAsync($base+'/status')) 8
            Say ('STATUS APOS ERRO: '+(Flat $after))
        }
    }catch{Say 'STATUS depois do erro indisponivel.'}
}finally{
    Poll-Serial
    if($null -ne $serial){if($serial.IsOpen){$serial.Close()};$serial.Dispose()}
    if($null -ne $payload){$payload.Dispose()}
    if($null -ne $client){$client.Dispose()}
    Say ('FIM. Linhas seriais capturadas='+$serialLines+'. Se falhar, copie as ultimas linhas da janela.')
}
