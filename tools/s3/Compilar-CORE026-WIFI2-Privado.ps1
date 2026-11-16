#requires -Version 5.1
<#
BOTIZIN CORE 0.0.26 Wi-Fi 2.0 — compilacao privada Windows.
Nenhuma gravacao na ESP32 e nenhuma publicacao no GitHub.
O token do dispositivo ThingsBoard e digitado APENAS no PC,
passa temporariamente como variavel de ambiente e nao deve ir ao chat.
Pre-requisitos: codigo extraido desta branch, Python 3.11,
Arduino CLI, esp32:esp32 3.3.12, PubSubClient 2.8.
#>
[CmdletBinding()]
param()
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
if(!(Test-Path (Join-Path $root 'ci\build.py'))){ throw 'Execute dentro da pasta completa do repositorio.' }
$versionFile=Join-Path $root 'firmware\version.h'
$policyFile=Join-Path $root 'firmware\lab_policy.h'
$sourceTokenFile=Join-Path $root 'firmware\telemetry_config.h'
if(!(Select-String -LiteralPath $versionFile -Pattern '#define BOTIZIN_VERSION "0.0.26"' -Quiet)){throw 'Versao incorreta; necessario 0.0.26.'}
if(!(Select-String -LiteralPath $policyFile -Pattern '#define BOTIZIN_LAB_OTA_DISABLED 0' -Quiet)){throw 'Politica OTA inesperada. Interrompido.'}
if(Test-Path $sourceTokenFile){throw 'Existe telemetry_config.h local. Remova-o manualmente somente apos verificar sua origem.'}
Write-Host 'BOTIZIN 0.0.26 - GERACAO PRIVADA SEM FLASH' -ForegroundColor Cyan
Write-Host 'Validando Python 3.11 e Arduino CLI...'
& py -3.11 --version
if($LASTEXITCODE -ne 0){throw 'Python 3.11 nao encontrado.'}
& arduino-cli version
if($LASTEXITCODE -ne 0){throw 'Instale arduino-cli 1.5.1 e coloque no PATH.'}
& arduino-cli core list
if($LASTEXITCODE -ne 0){throw 'Nao foi possivel verificar o core ESP32.'}
Write-Host 'Se nao tiver esp32:esp32 3.3.12 e PubSubClient 2.8, instale-os antes desta etapa.'
$secure=Read-Host 'Cole LOCALMENTE o token de dispositivo do ThingsBoard (nunca a API key de usuario)' -AsSecureString
$ptr=[IntPtr]::Zero
$code=1
try {
    $ptr=[Runtime.InteropServices.Marshal]::SecureStringToBSTR($secure)
    $plain=[Runtime.InteropServices.Marshal]::PtrToStringBSTR($ptr)
    if($plain -notmatch '^[A-Za-z0-9_-]{16,128}$' -or $plain.StartsWith('tb_')){
        throw 'Formato de token inesperado; interrompendo sem compilar.'
    }
    $env:BOTIZIN_TB_DEVICE_TOKEN=$plain
    $plain=$null
    Push-Location $root
    try {
        & py -3.11 (Join-Path $root 'ci\build.py')
        $code=$LASTEXITCODE
    } finally {
        Pop-Location
    }
    if($code -ne 0){throw 'Compilacao falhou; nenhuma imagem gerada para flash.'}
    $bin=Join-Path $root 'dist\firmware.bin'
    $hashFile=Join-Path $root 'dist\firmware.bin.sha256'
    $recordFile=Join-Path $root 'dist\build-record.json'
    if(!(Test-Path $bin) -or !(Test-Path $hashFile) -or !(Test-Path $recordFile)){throw 'Saidas de compilacao incompletas.'}
    $computed=(Get-FileHash -Algorithm SHA256 -LiteralPath $bin).Hash.ToLowerInvariant()
    $expected=((Get-Content -LiteralPath $hashFile -Raw).Trim()).ToLowerInvariant()
    if($computed -ne $expected){throw 'SHA256 divergente. Firmware privado nao sera usado.'}
    $record=Get-Content $recordFile -Raw | ConvertFrom-Json
    if($record.version -ne '0.0.26' -or !$record.publishable -or !$record.device_token_compiled){
        throw 'Metadados do build nao confirmam a configuracao privada.'
    }
    $private=Join-Path $env:USERPROFILE 'Downloads\BOTIZIN-RECUPERACAO-S3\FIRMWARE-PRIVADO'
    New-Item -ItemType Directory -Force -Path $private | Out-Null
    $out=Join-Path $private ('CORE-026-WIFI2-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
    New-Item -ItemType Directory -Path $out -ErrorAction Stop | Out-Null
    Copy-Item -LiteralPath $bin -Destination (Join-Path $out 'firmware.bin')
    Copy-Item -LiteralPath $hashFile -Destination (Join-Path $out 'firmware.bin.sha256')
    Copy-Item -LiteralPath $recordFile -Destination (Join-Path $out 'build-record.json')
    Write-Host 'FIRMWARE_026_PRIVADO_OK' -ForegroundColor Green
    Write-Host ('TAMANHO_BYTES: '+(Get-Item -LiteralPath $bin).Length)
    Write-Host ('SHA256: '+$computed)
    Write-Host ('LOCAL_PRIVADO: '+$out)
    Write-Host 'NENHUM_FLASH_EXECUTADO. NENHUM_RELEASE_PUBLICADO.'
} finally {
    Remove-Item Env:BOTIZIN_TB_DEVICE_TOKEN -ErrorAction SilentlyContinue
    if($ptr -ne [IntPtr]::Zero){[Runtime.InteropServices.Marshal]::ZeroFreeBSTR($ptr)}
    if(Test-Path $sourceTokenFile){Remove-Item -LiteralPath $sourceTokenFile -Force -ErrorAction SilentlyContinue}
    # Artefatos compilados no checkout tambem incorporam a credencial e nao
    # devem ser submetidos em commit nem enviados ao GitHub.
    foreach($local in @('dist\firmware.bin','dist\firmware.elf','compiler-output\firmware.ino.bin','compiler-output\firmware.ino.elf')){
        $p=Join-Path $root $local
        if(Test-Path $p){Remove-Item -LiteralPath $p -Force -ErrorAction SilentlyContinue}
    }
}
