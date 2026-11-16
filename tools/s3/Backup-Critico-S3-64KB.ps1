#requires -Version 5.1
<#
BOTIZIN S3 - Backup da area de boot/configuracao (0x00000..0x0FFFF).
LEITURA SOMENTE: nao apaga, nao grava e nao instala firmware.
Contem NVS com possiveis senhas e tokens: manter PRIVADO, nao enviar em chats/GitHub.
NAO e backup integral dos 16 MB nem da imagem app0.
#>
param([string]$Porta = "COM4")
$ErrorActionPreference = "Stop"

$modules = Join-Path $env:LOCALAPPDATA "BOTIZIN\tools\esptool-5.1.0-py311"
if (-not (Test-Path -LiteralPath $modules -PathType Container)) {
    throw "Nao encontrei o esptool previamente instalado em: $modules"
}
$env:PYTHONPATH = $modules

& py -3.11 -c "import esptool; print('ESPTOOL_PRONTO:', esptool.__version__)"
if ($LASTEXITCODE -ne 0) {
    throw "Python 3.11 ou esptool indisponivel. Nenhuma leitura realizada."
}

$pasta = Join-Path $env:USERPROFILE "Downloads\BOTIZIN-RECUPERACAO-S3\BACKUP-PRIVADO"
New-Item -ItemType Directory -Path $pasta -Force | Out-Null
$data = Get-Date -Format "yyyyMMdd-HHmmss"
$final = Join-Path $pasta ("S3-BOOT-NVS-OTADATA-64KB-" + $data + ".bin")
$parcial = "$final.partial"
if ((Test-Path -LiteralPath $final) -or (Test-Path -LiteralPath $parcial)) {
    throw "Arquivo ja existe; nao vou sobrescrever nada."
}

Write-Host "LEITURA APENAS: primeiros 65536 bytes da flash da S3 na $Porta"
Write-Host "Feche monitores USB antes de executar. O USB pode reiniciar."
Write-Host "Este arquivo pode conter credenciais. NAO COMPARTILHE."
& py -3.11 -m esptool --chip esp32s3 --port $Porta --baud 115200 --after hard-reset read-flash 0x0 0x10000 $parcial
if ($LASTEXITCODE -ne 0) {
    throw "FALHA_NA_LEITURA: nao confiar no arquivo parcial. Nenhuma gravacao foi executada."
}
if (-not (Test-Path -LiteralPath $parcial -PathType Leaf)) {
    throw "FALHA: esptool nao produziu arquivo."
}
$tamanho = (Get-Item -LiteralPath $parcial).Length
if ($tamanho -ne 65536) {
    throw "FALHA: arquivo tem $tamanho bytes, esperado 65536. Nao confiar no parcial."
}
Move-Item -LiteralPath $parcial -Destination $final -ErrorAction Stop
$hash = (Get-FileHash -LiteralPath $final -Algorithm SHA256).Hash
Write-Host "BACKUP_CRITICO_64KB_OK"
Write-Host "TAMANHO: 65536 bytes"
Write-Host "SHA256: $hash"
Write-Host "GUARDADO EM PASTA PRIVADA; nao compartilhe o arquivo."
Write-Host "AVISO: backup parcial de boot/NVS/otadata, NAO backup integral de flash."
