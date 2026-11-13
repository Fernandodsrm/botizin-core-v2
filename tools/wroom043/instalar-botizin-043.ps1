#requires -Version 5.1
# WROOM 0.0.43 staged stable OLED + XiaoZhi, USB only.
# Never overwrites app0, NVS, partitions or ESP32-S3. Private 4 MB backup.
param(
 [Parameter(Mandatory=$true)][string]$SourceRevision,
 [string]$Port='COM6',
 [string]$Confirm=''
)
$ErrorActionPreference='Stop'
$ProgressPreference='SilentlyContinue'
[Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12
function Log([string]$s){Write-Host '';Write-Host $s -ForegroundColor Cyan}
function Wroom(){return (Invoke-WebRequest 'http://192.168.0.23/status' -UseBasicParsing -TimeoutSec 6).Content}
try{
 if($Confirm -ne 'INSTALAR_WROOM_043_OTA1'){throw 'Falta confirmacao explicita: INSTALAR_WROOM_043_OTA1'}
 if($SourceRevision -notmatch '^[0-9a-f]{40}$'){throw 'Commit fixado invalido'}
 if($Port -notmatch '^COM[0-9]+$'){throw 'Porta COM invalida'}
 $root=Join-Path $env:USERPROFILE 'BOTIZIN_WROOM_STABLE_043'
 $backup=Join-Path $root 'backup-privado'
 $bin=Join-Path $root 'firmware-wroom-043.bin'
 $installer=Join-Path $root 'install_stable.py'
 $venv=Join-Path $root '.venv'
 $py=Join-Path $venv 'Scripts\python.exe'
 New-Item -ItemType Directory -Path $backup -Force | Out-Null

 Log '[1/7] PREVIA: IDENTIDADE DA S3 E WROOM'
 $s3=Invoke-RestMethod 'http://192.168.0.24/status' -TimeoutSec 6
 if($s3.origin -ne 'XIAOZHI_REAL'){throw 'Identidade S3 inesperada'}
 $old=Wroom
 $pilot=($old.StartsWith(('BOTIZIN WROOM V0.0.42'+[char]10)) -and
       $old -match 'RUNNING_PARTITION: app1 / ota_1' -and
       $old -match 'BOOT_PARTITION: app0 / ota_0')
 $base=($old.StartsWith(('BOTIZIN WROOM V0.0.40'+[char]10)) -and
       $old -match 'RUNNING_PARTITION: app0 / ota_0' -and
       $old -match 'BOOT_PARTITION: app0 / ota_0')
 if(!$pilot -and !$base){throw 'WROOM nao esta no piloto .42 com retorno armado nem na base .40; nao gravar'}
 Write-Host ('Estado anterior: '+(($old -split '\r?\n')[0])) -ForegroundColor Green

 Log '[2/7] BINARIO ASSINADO PELO SHA-256 E INSTALADOR FIXADO'
 $raw="https://raw.githubusercontent.com/Fernandodsrm/botizin-core-v2/$SourceRevision"
 Invoke-WebRequest "$raw/lab-builds/wroom-voice-stable-043/firmware.bin" -UseBasicParsing -OutFile $bin -TimeoutSec 90
 Invoke-WebRequest "$raw/tools/wroom043/install_stable.py" -UseBasicParsing -OutFile $installer -TimeoutSec 45
 $expect='ddeb66420f3cb85873938db54f885254bcd364d8d5b1a7c70542b1b60ead4b14'
 if(((Get-FileHash $bin -Algorithm SHA256).Hash.ToLowerInvariant()) -ne $expect -or
    (Get-Item $bin).Length -ne 1104848){
     throw 'Firmware divergiu do SHA pinado. STOP.'
 }
 Write-Host 'Firmware .43: tamanho e SHA256 confirmados' -ForegroundColor Green

 Log '[3/7] PYTHON USB ISOLADO'
 if(!(Test-Path $py)){
    & py -3.11 -m venv $venv
    if($LASTEXITCODE -ne 0){throw 'Python 3.11/venv ausente'}
 }
 $probe=& $py -c "import importlib.util; print('YES' if importlib.util.find_spec('esptool') and importlib.util.find_spec('serial') else 'NO')"
 if(($probe -join '') -ne 'YES'){
    & $py -m pip --disable-pip-version-check install 'esptool>=5,<6' 'pyserial>=3.5,<4'
    if($LASTEXITCODE -ne 0){throw 'Falha dependencias USB'}
 }
 & $py -m py_compile $installer
 if($LASTEXITCODE -ne 0){throw 'Sintaxe instalador invalida'}

 Log '[4/7] IDENTIFICACAO FISICA DA WROOM COM6'
 # Read-only esptool command hard-resets target. If .42 is active it should boot base .40.
 $flash=(& $py -m esptool --chip esp32 --port $Port --baud 230400 flash-id 2>&1 | Out-String)
 if($LASTEXITCODE -ne 0){throw ('Falha identificacao USB; '+$flash.Substring(0,[Math]::Min($flash.Length,250)))}
 if($flash -notmatch '30:76:f5:90:67:e0' -or $flash -notmatch '4MB'){
     throw 'MAC/flash da COM nao bate com WROOM; STOP sem gravar'
 }
 Write-Host 'MAC 30:76:f5:90:67:e0, flash 4MB' -ForegroundColor Green
 if($pilot){
   Log 'RETORNANDO PILOTO .42 A BASE .40 POR RESET USB'
   $confirmBase=$false
   for($i=0;$i -lt 18;$i++){
     try{
       $current=Wroom
       if($current.StartsWith(('BOTIZIN WROOM V0.0.40'+[char]10)) -and
          $current -match 'RUNNING_PARTITION: app0 / ota_0' -and
          $current -match 'BOOT_PARTITION: app0 / ota_0'){
          $confirmBase=$true;break
       }
     } catch{}
     Start-Sleep -Seconds 2
   }
   if(!$confirmBase){throw 'Nao confirmei base .40 apos reset; NADA gravado'}
 }
 $baseStatus=Wroom
 if(!$baseStatus.StartsWith(('BOTIZIN WROOM V0.0.40'+[char]10)) -or
    $baseStatus -notmatch 'RUNNING_PARTITION: app0 / ota_0'){
    throw 'Base OTA0 nao conferiu. STOP'
 }

 Log '[5/7] BACKUP INTEGRAL + GRAVACAO APENAS APP1'
 Write-Host 'Nao desconecte o cabo USB enquanto o instalador estiver gravando' -ForegroundColor Yellow
 & $py -u $installer --port $Port --package $bin --backup-dir $backup --confirm PROMOTE_WROOM_APP1_043
 if($LASTEXITCODE -ne 0){throw 'Instalador parou. Nao repita automaticamente.'}

 Log '[6/7] AGUARDANDO CONFIRMACAO SAUDAVEL DA .43'
 $good=$false
 for($i=0;$i -lt 65;$i++){
   try{
     $now=Wroom
     if($now.StartsWith(('BOTIZIN WROOM V0.0.43'+[char]10)) -and
        $now -match 'RUNNING_PARTITION: app1 / ota_1' -and
        $now -match 'BOOT_PARTITION: app1 / ota_1' -and
        $now -match 'VOICE_STABLE_BOOT: CONFIRMED_APP1' -and
        $now -match 'VOICE_S3_POLL_OK:\s*[1-9][0-9]*'){
        $good=$true;break
     }
   } catch{}
   Start-Sleep -Seconds 2
 }
 if(!$good){throw 'APP1 ainda nao se confirmou: NAO REINICIE sem consultar /status e o backup'}

 Log '[7/7] SUCESSO: BOTIZIN WROOM .43 PERSISTENTE NA OTA1'
 Write-Host 'PROXIMO BOOT: APP1 .43; BASE APP0 .40 PRESERVADA' -ForegroundColor Green
 Write-Host 'ROSTO: estados S3 por WiFi; nenhuma dependencia A22 para animacao'
 Write-Host 'BACKUP 4MB PRIVADO EM:' $backup
 Write-Host 'IMPORTANTE: a central MCP Windows antiga reconhece .42 e ainda precisa atualizar allowlist .43.'
} catch{
 Write-Host ('PARE: '+$_.Exception.Message) -ForegroundColor Red
 Write-Host 'NAO EXECUTE NOVAMENTE SEM VERIFICAR /status E LOGS. APP0 BASE .40 DEVE SER PRESERVADA.'
 exit 1
}
