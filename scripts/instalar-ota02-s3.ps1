# OTA-02 experimental S3: installation via LAN; DOES NOT change GitHub manifest.
# Requires extracted GitHub Actions artifact containing firmware.bin and build-record.json.
[CmdletBinding()]
param(
  [Parameter(Mandatory=$true)][string]$PackageFolder,
  [string]$Device = "botizin-s3.local"
)
$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest
function Fail([string]$s) { throw "PARADO: $s" }
function FetchStatus([string]$base) {
  return (Invoke-WebRequest -UseBasicParsing -Uri ($base+"/status") -TimeoutSec 8).Content
}
$base="http://"+$Device
$bin=Join-Path $PackageFolder "firmware.bin"
$recordFile=Join-Path $PackageFolder "build-record.json"
if (-not (Test-Path -LiteralPath $bin)) { Fail "firmware.bin nao encontrado." }
if (-not (Test-Path -LiteralPath $recordFile)) { Fail "build-record.json nao encontrado." }

$record=Get-Content -LiteralPath $recordFile -Raw | ConvertFrom-Json
if ($record.version -ne "0.0.18" -or $record.core -ne "3.3.12" -or
    $record.published -ne $false -or $record.publishable -ne $false) {
  Fail "Pacote diferente do LAB 0.0.18 esperado."
}
$bytes=(Get-Item -LiteralPath $bin).Length
$sha=(Get-FileHash -LiteralPath $bin -Algorithm SHA256).Hash.ToLowerInvariant()
if ($bytes -ne 1209136 -or
    $sha -ne "a360836ac1d886e53ab1db90fe4046675c6904d3635815a0aab88696ff03300d" -or
    [string]$record.source_commit -ne "cb5a9aa713bd616409d70dc3c025e48f236d5596" -or
    $bytes -ne [long]$record.bytes -or
    $sha -ne [string]$record.sha256 -or $bytes -gt 3145728) {
  Fail "Tamanho ou SHA256 nao bate com registro de compilacao."
}
Write-Host ("Pacote 0.0.18: "+$bytes+" bytes; SHA256 "+$sha) -ForegroundColor Green
Write-Host "IMPORTANTE: pacote de LAB com token ThingsBoard de teste (nao funcional)."

$prodText=(Invoke-WebRequest -UseBasicParsing -Uri "https://raw.githubusercontent.com/Fernandodsrm/botizin-core-v2/main/manifest.json" -TimeoutSec 15).Content
$prod=$prodText | ConvertFrom-Json
if ($prod.version -ne "0.0.17" -or
    $prod.sha256 -ne "7a7c16bcb2acd885c74c7d8ad77fe15f9d0e1e55d634dfd23d1ebb7abdb6884f") {
  Fail "Manifesto da producao mudou. Pare e revise."
}
Write-Host "Manifesto principal continua 0.0.17." -ForegroundColor Green

try { $status=FetchStatus $base }
catch { Fail "Nao consigo acessar /status da S3 pela rede do notebook." }
if (-not $status.Contains("BOTIZIN CORE V0.0.17") -or
    -not $status.Contains("RUNNING_OTA_STATE: VALID") -or
    -not $status.Contains("OTA: READY") -or
    -not $status.Contains("NEXT_UPDATE_PARTITION:")) {
  Write-Host $status
  Fail "Placa nao esta com 0.0.17 VALID/READY ou status inesperado."
}
Write-Host "S3 local validada: 0.0.17, OTA READY, VALID." -ForegroundColor Green
Write-Host "Nao desligue a S3 durante a gravacao. Fonte USB estavel e necessaria."
Write-Host "Firmware 0.0.18 so sera considerado VALID se passar nos testes de saude."
$yes=Read-Host "Para gravar o slot inativo, digite INSTALAR"
if ($yes -cne "INSTALAR") { Write-Host "Cancelado. Nenhuma gravacao."; exit 0 }

$url=$base+"/update?size="+$bytes+"&sha256="+$sha
$curl=(Get-Command curl.exe -ErrorAction Stop).Source
Write-Host "Enviando firmware ao slot inativo..."
$response=& $curl -f -sS --connect-timeout 10 --max-time 180 --request POST --form ("firmware=@"+$bin+";type=application/octet-stream") $url 2>&1
if ($LASTEXITCODE -ne 0) { Fail ("Upload falhou: "+($response -join " ")) }
$body=$response -join [Environment]::NewLine
Write-Host $body
if (-not $body.Contains("OTA_WRITE_VERIFIED: YES") -or
    -not $body.Contains("SHA_CALCULATED_FLASH: "+$sha)) {
  Fail "Nao foi comprovada gravacao/hash Flash. Nao reiniciar."
}
Write-Host "Hash da Flash verificado." -ForegroundColor Green
$reboot=Read-Host "Para iniciar a 0.0.18 experimental digite REINICIAR"
if ($reboot -cne "REINICIAR") {
  Write-Host "Sem reboot por comando, mas nova imagem pode iniciar num proximo reset."
  exit 0
}
Invoke-WebRequest -UseBasicParsing -Method POST -Uri ($base+"/reboot") -TimeoutSec 12 | Out-Null
Write-Host "Reboot pedido. Monitorando o /status local..." -ForegroundColor Cyan
$deadline=(Get-Date).AddMinutes(5)
$seen018=$false
do {
  Start-Sleep -Seconds 5
  try {
    $st=FetchStatus $base
    if ($st.Contains("BOTIZIN CORE V0.0.18")) {
      $seen018=$true
      $lines=($st -split "[\r\n]+") | Where-Object { $_ -match "^(BOTIZIN CORE|RUNNING_PARTITION:|RUNNING_OTA_STATE:|OTA_BOOT_HEALTH:|WIFI:|OTA:|INTERNET_OTA:)" }
      Write-Host ($lines -join [Environment]::NewLine)
      if ($st.Contains("RUNNING_OTA_STATE: VALID") -and
          $st.Contains("OTA_BOOT_HEALTH: VALID_CONFIRMED_AFTER_WIFI_NTP_HTTP_MQTT")) {
        Write-Host "OTA-02: 0.0.18 saudavel, confirmado VALID depois dos testes." -ForegroundColor Green
        exit 0
      }
    } elseif ($st.Contains("BOTIZIN CORE V0.0.17")) {
      if ($seen018) {
        Write-Host "OTA-02: rollback observado de 0.0.18 para 0.0.17." -ForegroundColor Yellow
        exit 0
      }
      Write-Host "S3 reporta 0.0.17. Aguardando distinguir fase do boot/rollback..."
    }
  } catch {
    Write-Host "Aguardando a S3 responder..."
  }
} while ((Get-Date) -lt $deadline)
Write-Host "Sem resultado conclusivo. Consulte o /status e nao repita instalacao automaticamente." -ForegroundColor Yellow
exit 1
