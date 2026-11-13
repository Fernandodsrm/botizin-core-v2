#requires -Version 5.1
# BOTIZIN WROOM 0.0.42: backup integral privado, MAC obrigatoria, OTA1 somente.
# S3/Chrome/MCP/Windows/A22 mantidos. Necessita WROOM fisicamente na USB.
param(
 [Parameter(Mandatory=$true)][string]$SourceRevision,
 [string]$Port='COM6'
)
$ErrorActionPreference='Stop'
$ProgressPreference='SilentlyContinue'
[Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12
function Etapa([string]$msg){Write-Host '';Write-Host $msg -ForegroundColor Cyan}
try {
 if($SourceRevision -notmatch '^[0-9a-f]{40}$'){throw 'Commit GitHub invalido'}
 if($Port -notmatch '^COM[0-9]+$'){throw 'Porta USB invalida'}
 $root=Join-Path $env:USERPROFILE 'BOTIZIN_WROOM_TESTE_042'
 $backup=Join-Path $root 'backup-local'
 $venv=Join-Path $root '.venv'
 $python=Join-Path $venv 'Scripts\python.exe'
 $bin=Join-Path $root 'firmware-lab-042.bin'
 $installer=Join-Path $root 'install_voice_face_pilot.py'
 New-Item -ItemType Directory -Force -Path $backup | Out-Null

 Etapa '[1/6] CONFERINDO PLACAS SEM ALTERAR NADA'
 $wroom=(Invoke-WebRequest 'http://192.168.0.23/status' -UseBasicParsing -TimeoutSec 8).Content
 if(!$wroom.StartsWith(('BOTIZIN WROOM V0.0.40'+[char]10)) -or
   $wroom -notmatch 'RUNNING_PARTITION: app0 / ota_0 / address=0x00010000' -or
   $wroom -notmatch 'BOOT_PARTITION: app0 / ota_0' -or
   $wroom -notmatch 'NEXT_UPDATE_PARTITION: app1 / ota_1' -or
   $wroom -notmatch 'OTA: READY'){
   throw 'WROOM nao esta na BASE .40 OTA0 pronta; nao gravar'
 }
 Write-Host 'WROOM .40 OTA0 confirmada' -ForegroundColor Green
 $s3=Invoke-RestMethod 'http://192.168.0.24/status' -TimeoutSec 8
 if($s3.origin -ne 'XIAOZHI_REAL'){throw 'S3 XiaoZhi nao confirmou origem real'}
 Write-Host 'S3 XiaoZhi LAB acessivel' -ForegroundColor Green

 Etapa '[2/6] BAIXANDO BINARIO E INSTALADOR POR COMMIT'
 $raw="https://raw.githubusercontent.com/Fernandodsrm/botizin-core-v2/$SourceRevision"
 Invoke-WebRequest "$raw/lab-builds/wroom-voice-face-042/firmware.bin" -UseBasicParsing -OutFile $bin -TimeoutSec 100
 Invoke-WebRequest "$raw/tools/wroom042/install_voice_face_pilot.py" -UseBasicParsing -OutFile $installer -TimeoutSec 60
 $shaExpected='e21b5e42bd6f10e9cff71d19a4cefd8d29b229bd63d1092aaa6a55caff977495'
 $shaActual=(Get-FileHash $bin -Algorithm SHA256).Hash.ToLowerInvariant()
 if($shaActual -ne $shaExpected -or (Get-Item $bin).Length -ne 1103792){
   throw 'Firmware .42 falhou SHA256/tamanho. NADA gravado'
 }
 Write-Host 'BINARIO LAB .42 SHA256 VALIDADO' -ForegroundColor Green

 Etapa '[3/6] PYTHON SERIAL EM AMBIENTE ISOLADO'
 if(!(Test-Path $python)){
   & py -3.11 -m venv $venv
   if($LASTEXITCODE -ne 0){throw 'Nao foi possivel criar Python 3.11 isolado'}
 }
 $probe=& $python -c "import importlib.util; print('OK' if (importlib.util.find_spec('esptool') and importlib.util.find_spec('serial')) else 'MISSING')"
 if(($probe -join '') -ne 'OK'){
   & $python -m pip --disable-pip-version-check install 'esptool>=5,<6' 'pyserial>=3.5,<4'
   if($LASTEXITCODE -ne 0){throw 'Nao foi possivel preparar esptool USB'}
 }
 & $python -m py_compile $installer
 if($LASTEXITCODE -ne 0){throw 'Instalador nao passou verificacao de sintaxe'}

 Etapa '[4/6] BACKUP INTEGRAL E GRAVACAO SOMENTE OTA1'
 Write-Host ("Porta "+$Port+": confirma MAC e flash 4 MB antes de gravar")
 Write-Host 'NAO desconecte a USB durante gravacao/leituras' -ForegroundColor Yellow
 & $python -u $installer --port $Port --package $bin --backup-dir $backup --confirm PILOT_WROOM_APP1_042
 if($LASTEXITCODE -ne 0){throw 'Piloto USB falhou. Confira log; nao repita cegamente'}

 Etapa '[5/6] VALIDANDO WROOM LAB E LEITURA S3'
 $status=$null
 for($i=0;$i -lt 8;$i++){
   try{
     $status=(Invoke-WebRequest 'http://192.168.0.23/status' -UseBasicParsing -TimeoutSec 4).Content
     if($status.StartsWith(('BOTIZIN WROOM V0.0.42'+[char]10))){break}
   }catch{}
   Start-Sleep -Seconds 2
 }
 if(!$status -or !$status.StartsWith(('BOTIZIN WROOM V0.0.42'+[char]10))){
   throw 'LAB gravada mas WROOM .42 nao confirmada. Sem resets cegos'
 }
 if($status -notmatch 'VOICE_LAB_RETURN_BASE_ARMED: YES'){
   throw 'LAB .42 ativa mas retorno OTA0 ainda nao confirmado. NAO reinicie'
 }
 $ok=0
 for($i=0;$i -lt 8;$i++){
   $state=(Invoke-WebRequest 'http://192.168.0.23/status' -UseBasicParsing -TimeoutSec 4).Content
   if($state -match 'VOICE_S3_POLL_OK:\s*([0-9]+)'){
     $ok=[int]$Matches[1]
     if($ok -gt 0){break}
   }
   Start-Sleep -Milliseconds 800
 }
 if($ok -lt 1){throw 'LAB .42 ativa mas polling S3 nao confirmado. Nao reflashe'}

 Etapa '[6/6] PRONTO PARA CONVERSAR E OBSERVAR O OLED'
 Write-Host 'WROOM LAB 0.0.42 ATIVA NA OTA1' -ForegroundColor Green
 Write-Host ('GET S3 /status OK: '+$ok) -ForegroundColor Green
 Write-Host 'VOZ: Chrome http://192.168.0.24/botizin'
 Write-Host 'OUVINDO 5 / FALANDO 6: OLED; independente de A22 e notebook'
 Write-Host 'PROXIMO REBOOT: retorno a BASE OTA0 selecionado'
 Write-Host ('BACKUP PRIVADO (NAO ENVIE): '+$backup)
} catch {
 Write-Host ('INSTALACAO INTERROMPIDA: '+$_.Exception.Message) -ForegroundColor Red
 Write-Host 'Nao execute novamente antes de ler o log e verificar status WROOM'
 exit 1
}
