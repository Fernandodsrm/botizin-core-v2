#requires -Version 5.1
# Encerra SOMENTE o MCP provisório criado pelo script BOTIZIN Windows.
$ErrorActionPreference = 'Stop'
$root=Join-Path $env:USERPROFILE 'BOTIZIN_WINDOWS_TEMP'
$pidFile=Join-Path $root 'windows-mcp.pid'
if(!(Test-Path $pidFile)){Write-Host 'Nenhuma sessao temporaria registrada.';return}
$savedPid=[int](Get-Content $pidFile -Raw).Trim()
$proc=Get-CimInstance Win32_Process -Filter "ProcessId = $savedPid" -ErrorAction SilentlyContinue
if(!$proc){
    Remove-Item $pidFile -Force
    Write-Host 'MCP temporario ja estava encerrado.'; return
}
if($proc.CommandLine -notlike '*windows_mcp.py*' -or
   $proc.CommandLine -notlike '*BOTIZIN_WINDOWS_TEMP*'){
    throw 'PID nao pertence ao MCP temporario. Nada foi encerrado.'
}
Stop-Process -Id $savedPid -ErrorAction Stop
Remove-Item $pidFile -Force
Write-Host 'MCP TEMPORARIO ENCERRADO; A22/8765/PLACAS NAO ALTERADOS.' -ForegroundColor Green
