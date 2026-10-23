# BOTIZIN XiaoZhi 0.0.31 -- OTA local, download in RAM (no disk needed).
# Windows PowerShell 5.1+ / ESP32-S3 CORE 0.0.25 at 192.168.0.18, COM4 optional serial monitor.
# Preserves ota_0, writes ota_1; reboot only when SHA and post-flash verification succeed.
$ErrorActionPreference='Stop'
[Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12
Add-Type -AssemblyName System.Net.Http
$base='http://botizin-s3.local'
$manifestUrl='https://raw.githubusercontent.com/Fernandodsrm/botizin-core-v2/main/releases/0.0.31/manifest.json'
$fwUrl='https://raw.githubusercontent.com/Fernandodsrm/botizin-core-v2/main/releases/0.0.31/firmware.bin'
$size=0
$sha=''
$serial=$null
$client=$null
$payload=$null
$pending=''
$serialLines=0
$captureBoot=$false
$capturedBootLines=0
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
            if($script:captureBoot -and $script:capturedBootLines -lt 240) { $script:capturedBootLines++; Say ('BOOT_RAW: '+$line) }
            elseif($line -match '(?i)OTA|UPDATE|DOWNLOAD|SHA|WRITE|RESET_REASON|INT_WDT|TASK_WDT|GURU MEDITATION|PANIC|FAIL|BOOT_PARTITION|REBOOT|[Ee]rror|HTTP_SERVER') { Say ('COM4: '+$line) }
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
    try { $before=Await-Poll ($client.GetStringAsync($base+'/status')) 15 }
    catch {
        Say 'Nome botizin-s3.local indisponivel; tentando IP local conhecido.'
        $base='http://192.168.0.18'
        $before=Await-Poll ($client.GetStringAsync($base+'/status')) 15
    }
    Say ('STATUS ANTES: '+(Flat $before))
    if($before -notmatch 'BOTIZIN CORE V0[.]0[.]25'){throw 'PARADO: placa nao esta no CORE 0.0.25. Se LAB 0.0.30 estiver ativa, primeiro retorne ao CORE pelo endpoint /lab/return-core com sua senha local. NENHUM firmware foi gravado.'}
    if($before -notmatch 'RUNNING_PARTITION:\s*app0\s*/\s*ota_0'){throw 'PARADO: CORE 0.0.25 nao esta no slot ota_0'}
    if($before -notmatch 'NEXT_UPDATE_PARTITION:\s*app1\s*/\s*ota_1'){throw 'PARADO: slot de destino nao e ota_1'}
    if($before -notmatch 'OTA:\s*READY'){throw 'PARADO: OTA da placa nao esta READY'}
    Say 'PREFLIGHT OK: CORE preservado em ota_0; destino ota_1.'
    Say 'Conferindo manifesto publicado para a nova LAB 0.0.31...'
    $mText=Await-Poll ($client.GetStringAsync($manifestUrl)) 20
    $manifest=$mText|ConvertFrom-Json
    if($manifest.version -ne '0.0.31' -or $manifest.board -ne 'esp32s3-n16r8') {throw 'STOP: manifesto incorreto'}
    $size=[int]$manifest.size
    $sha=[string]$manifest.sha256
    # Pinned independently to the rebuilt private seven-part image (LAB 0.0.31 release).
    if($size -ne 1951168 -or $sha -ne 'adffd5c3208c6a9701670f204b884c764b1688c8c8169e89f3bbec0fe354e26f') {throw 'STOP: manifesto mudou ou aponta para firmware diferente da LAB 0.0.31 aprovada'}
    if($size -lt 1000000 -or $size -gt 3145728 -or $sha -notmatch '^[0-9a-f]{64}$' -or $manifest.url -ne $fwUrl) {throw 'STOP: manifesto de 0.0.31 invalido'}
    Say ('LAB 0.0.31 CONFIRMADA: bytes='+$size+' SHA='+$sha)
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
    $script:captureBoot=$true
    Say 'Captura serial COMPLETA ativada para os primeiros 240 logs de inicializacao.'
    $empty=[System.Net.Http.ByteArrayContent]::new([byte[]]@())
    $restart=Await-Poll ($client.PostAsync($base+'/reboot',$empty)) 15
    Say ('REBOOT HTTP='+[int]$restart.StatusCode)
    if(!$restart.IsSuccessStatusCode){throw 'OTA gravado mas reboot nao confirmado; nao insistir sem conferir status'}
    Say 'Aguardando inicializacao XiaoZhi em /botizin (primeiro boot). NOVO: finalize a fala sem fechar o socket.'
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
    if($found){
        Say 'SUCESSO: XiaoZhi 0.0.31 respondeu. Abrindo navegador de microfone.'
        $apps=@(
            'C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe',
            'C:\Program Files\Microsoft\Edge\Application\msedge.exe',
            'C:\Program Files\Google\Chrome\Application\chrome.exe',
            'C:\Program Files (x86)\Google\Chrome\Application\chrome.exe'
        )
        $browser=$apps|Where-Object{Test-Path $_}|Select-Object -First 1
        if($browser){
            $profile=if(Test-Path 'D:\'){'D:\BOTIZIN-Browser-Mic-LAB'}else{(Join-Path $env:TEMP 'BOTIZIN-Browser-Mic-LAB')}
            New-Item -ItemType Directory -Force -Path $profile|Out-Null
            $args=@(
                ('--user-data-dir="'+$profile+'"'),
                '--unsafely-treat-insecure-origin-as-secure=http://192.168.0.18',
                '--no-first-run',
                '--new-window',
                'http://192.168.0.18/botizin'
            )
            Start-Process -FilePath $browser -ArgumentList $args
            Say 'NAVEGADOR ABERTO: clique Conversar com XiaoZhi e permita o microfone. Teste local somente.'
        } else {
            Say 'Edge/Chrome nao localizado; navegador de laboratorio nao aberto.'
        }
    }
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
