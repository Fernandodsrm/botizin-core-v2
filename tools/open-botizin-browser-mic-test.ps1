# BOTIZIN browser microphone laboratory test: Windows, Edge/Chrome only.
# One local HTTP origin is allowed to access microphone in isolated browser profile.
# DEVELOPMENT ONLY. Use valid HTTPS for permanent mobile voice access.
$ErrorActionPreference='Stop'
$origin='http://192.168.0.18'
$profile=if(Test-Path 'D:\'){'D:\BOTIZIN-Browser-Mic-LAB'}else{(Join-Path $env:TEMP 'BOTIZIN-Browser-Mic-LAB')}
$apps=@(
 'C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe',
 'C:\Program Files\Microsoft\Edge\Application\msedge.exe',
 'C:\Program Files\Google\Chrome\Application\chrome.exe',
 'C:\Program Files (x86)\Google\Chrome\Application\chrome.exe'
)
$browser=$apps|Where-Object{Test-Path $_}|Select-Object -First 1
if(!$browser){throw 'Edge ou Chrome nao encontrado'}
New-Item -ItemType Directory -Force -Path $profile|Out-Null
$args=@(
 ('--user-data-dir="'+$profile+'"'),
 ('--unsafely-treat-insecure-origin-as-secure='+$origin),
 '--no-first-run',
 '--new-window',
 ($origin+'/botizin')
)
Write-Host 'Abrindo navegador exclusivo com microfone liberado SOMENTE para 192.168.0.18.' -ForegroundColor Cyan
Write-Host 'Nao navegue para outros sites com esse perfil de laboratorio.'
Start-Process -FilePath $browser -ArgumentList $args
