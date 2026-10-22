# BOTIZIN XiaoZhi native - one-shot OTA installer, Windows PowerShell 5.1
# Exact CI artifact (private repository) and SHA pinned. NEVER flash bootloader/partition table.
$ErrorActionPreference = 'Stop'
$Target = 'http://192.168.0.18'
$ArtifactUrl = 'https://github.com/Fernandodsrm/botizin-core/actions/runs/39860397833/artifacts/12461578206'
$ExpectedSha = 'c4a83a5a41de7b84c9afd088c0d81decec2563f406506d88ec2873cfe7f658c4'
$ExpectedSize = 1942976
$Folder = Join-Path $env:USERPROFILE 'Downloads'
$Staging = Join-Path $env:TEMP 'botizin-xiaozhi-ci-39860397833'
$ZipPattern = 'botizin-xiaozhi-native-web-lab*.zip'
Write-Host '=== BOTIZIN XiaoZhi REAL / LAB ===' -ForegroundColor Cyan
Write-Host 'Instalacao autorizada: apenas firmware em OTA_1. Risco de precisar apertar BOOT fisicamente.' -ForegroundColor Yellow

# Preflight is automatic, not separate user actions.
$before = (Invoke-WebRequest -UseBasicParsing -TimeoutSec 12 -Uri "$Target/status").Content
if($before -notmatch 'BOTIZIN CORE V0\.0\.25') { throw 'ABORTADO: a S3 nao esta rodando CORE 0.0.25.' }
if($before -notmatch 'RUNNING_PARTITION:\s*app0\s*/\s*ota_0') { throw 'ABORTADO: CORE nao esta em ota_0.' }
if($before -notmatch 'NEXT_UPDATE_PARTITION:\s*app1\s*/\s*ota_1') { throw 'ABORTADO: proximo slot nao e ota_1.' }
if($before -notmatch 'NEXT_UPDATE_PARTITION:.*size=3145728') {
  throw 'ABORTADO: nao confirmou slot OTA de 3MiB.'
}
Write-Host 'CORE 0.0.25 em ota_0, destino ota_1: OK.' -ForegroundColor Green

# The artifact is in a private repo, download with the browser login, no PAT/gh/Python.
function Find-ExactArtifact {
  $zips = @(Get-ChildItem -LiteralPath $Folder -Filter $ZipPattern -File -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending)
  foreach($z in $zips){
    $tmp = Join-Path $Staging ('unpack-'+$z.LastWriteTime.Ticks)
    try {
      if(Test-Path $tmp){ Remove-Item -LiteralPath $tmp -Force -Recurse }
      New-Item -ItemType Directory -Path $tmp -Force | Out-Null
      Expand-Archive -LiteralPath $z.FullName -DestinationPath $tmp -Force -ErrorAction Stop
      $bins = @(Get-ChildItem -LiteralPath $tmp -Recurse -Filter 'xiaozhi.bin' -File)
      foreach($bin in $bins){
        if($bin.Length -ne $ExpectedSize){ continue }
        $hash = (Get-FileHash -LiteralPath $bin.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        if($hash -eq $ExpectedSha){ return $bin.FullName }
      }
    } catch { }
  }
  return $null
}
New-Item -ItemType Directory -Path $Staging -Force | Out-Null
$firmware = Find-ExactArtifact
if(-not $firmware){
  Write-Host 'Abrindo o artefato no navegador do notebook...' -ForegroundColor Cyan
  Start-Process $ArtifactUrl
  Write-Host 'Entre no GitHub, se solicitado, e baixe o ZIP da compilacao. O instalador reconhece o arquivo automaticamente.' -ForegroundColor Yellow
  $limit=(Get-Date).AddMinutes(6)
  do {
    Start-Sleep -Seconds 4
    $firmware = Find-ExactArtifact
  } until($firmware -or (Get-Date) -ge $limit)
  if(-not $firmware){ throw "Artefato correto nao encontrado em Downloads. Link: $ArtifactUrl" }
}
$size = (Get-Item -LiteralPath $firmware).Length
$sha = (Get-FileHash -LiteralPath $firmware -Algorithm SHA256).Hash.ToLowerInvariant()
if($sha -ne $ExpectedSha -or $size -ne $ExpectedSize){throw 'ERRO: hash/tamanho do firmware nao correspondem ao build aprovado.'}
Write-Host "BIN confirmado: $firmware" -ForegroundColor Green
Write-Host "Tamanho: $size | SHA256: $sha"

# CORE 0.0.25 WebServer requires multipart, not raw binary.
$uploadUrl = "$Target/update?size=$size&sha256=$sha"
Write-Host 'Gravando apenas firmware de aplicativo em OTA_1...' -ForegroundColor Cyan
$response = & curl.exe --silent --show-error --fail --connect-timeout 8 --max-time 180 -X POST -F "firmware=@$firmware;type=application/octet-stream" "$uploadUrl"
$rc=$LASTEXITCODE
if($rc -ne 0){throw "Upload falhou (curl $rc). NAO solicitando reboot."}
if(($response -join [Environment]::NewLine) -notmatch 'OTA_WRITE_VERIFIED:\s*YES') {
  Write-Host ($response -join [Environment]::NewLine)
  throw 'Upload nao confirmou OTA_WRITE_VERIFIED: YES. NAO reiniciando.'
}
Write-Host 'Flash OTA_1 e SHA verificados pelo CORE 0.0.25.' -ForegroundColor Green

$after=(Invoke-WebRequest -UseBasicParsing -TimeoutSec 12 -Uri "$Target/status").Content
if($after -notmatch 'BOOT_PARTITION:\s*app1\s*/\s*ota_1'){
  throw 'Nao confirmou boot do ota_1. NAO reiniciando.'
}
Write-Host 'Reiniciando uma vez para iniciar o XiaoZhi...' -ForegroundColor Cyan
$result=& curl.exe --silent --show-error --connect-timeout 8 --max-time 15 --request POST "$Target/reboot"
if($LASTEXITCODE -ne 0){throw 'Nao confirmou comando de reinicio.'}
Write-Host ($result -join [Environment]::NewLine)
Write-Host 'Verificando a pagina do XiaoZhi...' -ForegroundColor Cyan

$deadline=(Get-Date).AddMinutes(2)
$seen=$false
do{
  Start-Sleep -Seconds 4
  try {
    $status=Invoke-RestMethod -TimeoutSec 5 -Uri "$Target/status"
    if($status.origin -eq 'XIAOZHI_REAL'){
      $seen=$true
      Write-Host 'XIAOZHI ONLINE NA S3: http://192.168.0.18/botizin' -ForegroundColor Green
      Write-Host "Estado: $($status.state) | Web audio: $($status.web_audio_connected)"
      Start-Process "$Target/botizin"
      break
    }
  }catch{}
} until((Get-Date) -gt $deadline)
if(-not $seen){
  Write-Host 'Novo painel nao confirmado. O LAB tenta retornar ao CORE em 4 minutos; nao comprovado fisicamente.' -ForegroundColor Yellow
  Write-Host 'Se nao retornar, a recuperacao pode exigir USB e o botao BOOT.' -ForegroundColor Yellow
}
Write-Host 'Este instalador NAO cria acesso HTTPS nem ativa o servidor XiaoZhi.' -ForegroundColor Yellow
