# Windows PowerShell 5.1. One authenticated command; subsequent requests only read status.
param([string]$DeviceUrl='http://192.168.0.7')
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Net.Http
$base=$DeviceUrl.TrimEnd('/')
$handler=[System.Net.Http.HttpClientHandler]::new()
$handler.UseProxy=$false
$client=[System.Net.Http.HttpClient]::new($handler)
$client.Timeout=[TimeSpan]::FromSeconds(12)
$secret=$null
$credential=$null
$message=$null
$reply=$null
try {
    # Refuse older firmware before asking for the local password or sending a command.
    $telemetry=($client.GetStringAsync($base+'/lab/telemetry').GetAwaiter().GetResult())|ConvertFrom-Json
    if($telemetry.lab -ne '0.0.36'){throw 'A S3 nao confirmou LAB 0.0.36. Nenhum comando foi enviado.'}
    $before=($client.GetStringAsync($base+'/lab/phone').GetAwaiter().GetResult())|ConvertFrom-Json
    if($before.running){throw 'Ja existe um comando em andamento. Nao foi enviado outro.'}
    Write-Host ('S3 -> A22: '+$before.target)
    Write-Host 'Deixe o A22 desbloqueado e a macro HTTP ativada. Feche os outros paineis BOTIZIN.'
    $secret=Read-Host 'Senha individual de configuracao da S3 (nao aparece na tela)' -AsSecureString
    $credential=[System.Management.Automation.PSCredential]::new('botizin',$secret)
    $message=[System.Net.Http.HttpRequestMessage]::new([System.Net.Http.HttpMethod]::Post,$base+'/lab/phone/open')
    $message.Headers.Add('X-Botizin-Key',$credential.GetNetworkCredential().Password)
    $message.Content=[System.Net.Http.ByteArrayContent]::new([byte[]]@())
    $reply=$client.SendAsync($message).GetAwaiter().GetResult()
    $text=$reply.Content.ReadAsStringAsync().GetAwaiter().GetResult()
    Write-Host ('S3 HTTP='+[int]$reply.StatusCode+' '+$text)
    if([int]$reply.StatusCode -ne 202){throw 'A S3 nao aceitou o comando. Confira a mensagem acima.'}
    $deadline=(Get-Date).AddSeconds(15)
    $result=$null
    do {
        Start-Sleep -Milliseconds 500
        $result=($client.GetStringAsync($base+'/lab/phone').GetAwaiter().GetResult())|ConvertFrom-Json
        if([int]$result.attempts -ne ([int]$before.attempts+1)){throw 'O contador de comandos mudou inesperadamente; confira o diagnostico antes de repetir.'}
    } while($result.running -and (Get-Date) -lt $deadline)
    $result|ConvertTo-Json -Depth 4
    if($result.running){throw 'O comando continua em andamento. Nao foi repetido.'}
    if($result.received){Write-Host 'A S3 recebeu 200 + BOTIZIN_COMANDO_RECEBIDO do A22. Confira se o painel abriu no celular.'}
    else {throw 'Nao houve confirmacao completa do A22. Confira o celular e envie a saida antes de repetir.'}
} catch {
    Write-Host ('ERRO: '+$_.Exception.Message)
    Write-Host 'Nao repetimos o comando automaticamente.'
    exit 1
} finally {
    if($null -ne $message){$message.Headers.Clear();$message.Dispose()}
    if($null -ne $reply){$reply.Dispose()}
    $credential=$null
    if($null -ne $secret){$secret.Dispose()}
    $client.Dispose()
}
