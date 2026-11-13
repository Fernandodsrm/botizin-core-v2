#requires -Version 5.1
# Central provisoria BOTIZIN no Windows; preserva A22, firmware, firewall e MCP original.
param([Parameter(Mandatory=$true)][string]$SourceRevision)
$ErrorActionPreference='Stop'
$ProgressPreference='SilentlyContinue'
[Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12
function Etapa([string]$s) { Write-Host ''; Write-Host $s -ForegroundColor Cyan }
function Aviso([string]$s) { Write-Host $s -ForegroundColor Yellow }

try {
    $root=Join-Path $env:USERPROFILE 'BOTIZIN_WINDOWS_TEMP'
    $venv=Join-Path $root '.venv'
    $python=Join-Path $venv 'Scripts\python.exe'
    $server=Join-Path $root 'windows_mcp.py'
    $stdout=Join-Path $root 'mcp-out.log'
    $stderr=Join-Path $root 'mcp-err.log'
    $pidFile=Join-Path $root 'windows-mcp.pid'
    New-Item -ItemType Directory -Path $root -Force | Out-Null

    Etapa '[1/5] Verificando a S3 e WROOM'
    try {
        $s3=Invoke-RestMethod 'http://192.168.0.24/status' -TimeoutSec 5
        if($s3.origin -ne 'XIAOZHI_REAL'){throw 'S3 identidade incorreta'}
        Write-Host ('S3: ONLINE. Estado: '+$s3.state) -ForegroundColor Green
    } catch { Aviso ('S3 sem confirmacao: '+$_.Exception.Message) }
    try {
        $wroom=(Invoke-WebRequest 'http://192.168.0.23/status' -UseBasicParsing -TimeoutSec 5).Content
        if(!$wroom.StartsWith('BOTIZIN WROOM V')){throw 'WROOM identidade incorreta'}
        $v=($wroom -split '\r?\n')[0]
        Write-Host $v -ForegroundColor Green
        if($v -match 'V0\.0\.40'){Aviso 'Olhos OLED ainda independentes: WROOM 0.0.40 sem receptor de estado.'}
    } catch { Aviso ('WROOM sem confirmacao: '+$_.Exception.Message) }

    Etapa '[2/5] Preparando Python em pasta isolada'
    if(!(Test-Path $python)){
        if(Get-Command py.exe -ErrorAction SilentlyContinue){
            & py -3.11 -m venv $venv
        } else {
            $cmd=Get-Command python.exe -ErrorAction Stop
            & $cmd.Source -m venv $venv
        }
        if($LASTEXITCODE -ne 0){throw 'Nao foi possivel criar venv Python.'}
    }

    $url="https://raw.githubusercontent.com/Fernandodsrm/botizin-core-v2/$SourceRevision/tools/windows_hub/windows_mcp.py"
    Invoke-WebRequest $url -OutFile $server -UseBasicParsing -TimeoutSec 30

    & $python $server --self-test
    if($LASTEXITCODE -ne 0){throw 'Testes offline do hub falharam'}
    & $python -m py_compile $server
    if($LASTEXITCODE -ne 0){throw 'Falha de sintaxe Python'}

    Etapa '[3/5] Checando SDK MCP v2'
    & $python -c "from mcp.server import MCPServer" 2>$null
    if($LASTEXITCODE -ne 0){
        Write-Host 'Instalando dependencias SOMENTE na venv isolada...'
        & $python -m pip --disable-pip-version-check install 'mcp>=2,<3'
        if($LASTEXITCODE -ne 0){throw 'Instalacao do SDK MCP falhou'}
        & $python -c "from mcp.server import MCPServer"
        if($LASTEXITCODE -ne 0){throw 'SDK MCP v2 indisponivel'}
    }

    Etapa '[4/5] Iniciando na porta 8766, somente 127.0.0.1'
    $connected=$false
    $sock=New-Object System.Net.Sockets.TcpClient
    try {
        $h=$sock.BeginConnect('127.0.0.1',8766,$null,$null)
        if($h.AsyncWaitHandle.WaitOne(800)){$sock.EndConnect($h);$connected=$true}
    } catch {} finally {$sock.Close()}

    if($connected){
        if(!(Test-Path $pidFile)){throw 'Porta 8766 ocupada por processo desconhecido.'}
        $savedPid=[int](Get-Content $pidFile -Raw).Trim()
        $proc=Get-CimInstance Win32_Process -Filter "ProcessId = $savedPid" -ErrorAction SilentlyContinue
        if(!$proc -or $proc.CommandLine -notlike '*windows_mcp.py*'){
            throw 'Porta 8766 ocupada por processo nao confirmado.'
        }
        Write-Host ('Hub anterior ja ativo. PID '+$savedPid) -ForegroundColor Green
    } else {
        $argsPython=@('-u',('"' + $server + '"'))
        $p=Start-Process -FilePath $python -ArgumentList $argsPython -WorkingDirectory $root -RedirectStandardOutput $stdout -RedirectStandardError $stderr -PassThru
        Set-Content -Path $pidFile -Value $p.Id -NoNewline
        $ready=$false
        for($i=0;$i -lt 25;$i++){
            Start-Sleep -Milliseconds 400
            $p.Refresh()
            if($p.HasExited){break}
            $sock2=New-Object System.Net.Sockets.TcpClient
            try{
                $h2=$sock2.BeginConnect('127.0.0.1',8766,$null,$null)
                if($h2.AsyncWaitHandle.WaitOne(300)){$sock2.EndConnect($h2);$ready=$true}
            }catch{}finally{$sock2.Close()}
            if($ready){break}
        }
        if(!$ready){
            if(Test-Path $stderr){Get-Content $stderr -Tail 12}
            throw 'MCP nao iniciou na 8766.'
        }
        Write-Host ('Hub iniciado. PID '+$p.Id) -ForegroundColor Green
    }

    Etapa '[5/5] CENTRAL PROVISORIA'
    Write-Host 'MCP WINDOWS LOCAL: http://127.0.0.1:8766/mcp' -ForegroundColor Green
    Write-Host 'MCP original 8765, A22, firmware e tunel: NAO ALTERADOS'
    Write-Host 'VOZ: mantenha a aba Chrome http://192.168.0.24/botizin'
    Write-Host ('PASTA: '+$root)
    Write-Host 'A35 remoto e plugin ChatGPT na nuvem exigem tunel autenticado adicional.'
    Write-Host 'WROOM .40 ainda precisa de LAB com receptor para olhos sincronizados.'
} catch {
    Write-Host ('FALHA SEGURA: '+$_.Exception.Message) -ForegroundColor Red
    exit 1
}
