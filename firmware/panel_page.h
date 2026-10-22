#pragma once
// Static flash-backed panel. No polling, timers or server-side dynamic page generation.
// /status and /events are read ONLY after the page loads once or a human clicks.
#include <pgmspace.h>
static const char kBotizinPanelHTML[] PROGMEM = R"BTZN(<!doctype html>
<html lang="pt-BR"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="color-scheme" content="dark"><title>BOTIZIN | Diagnóstico</title>
<style>
:root{font-family:system-ui,-apple-system,Segoe UI,sans-serif;color:#e4f2ed;background:#0d1918}
*{box-sizing:border-box}body{margin:0}main{max-width:1060px;margin:0 auto;padding:18px}
header{display:flex;justify-content:space-between;align-items:center;gap:12px;padding:18px 0 25px}
h1{font-size:1.6rem;margin:0;font-weight:790;letter-spacing:-.05em}h2{font-size:1.04rem;margin:0 0 8px}
p{line-height:1.55}.muted{color:#a3b7b0}.small{font-size:.83rem}
.logo{color:#91fac9;font-weight:800;font-size:.78rem;letter-spacing:.14em}
.flag{font-size:.78rem;border:1px solid #3b6052;color:#a8dac4;border-radius:30px;padding:7px 10px}
.description{margin:4px 0 20px;font-size:.92rem;color:#a8bbb3}
.g{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:11px}
.metric,.panel{background:#132522;border:1px solid #315046;border-radius:14px;padding:15px;min-width:0}
.metric .k{color:#a3b7b0;font-size:.78rem;margin-bottom:9px}
.metric .v{font-size:1.05rem;font-weight:720;overflow-wrap:anywhere}
main section{margin-top:14px}
.pair{display:grid;grid-template-columns:minmax(0,1fr) minmax(0,1fr);gap:14px}
.pill{font-size:.83rem;border-radius:99px;padding:6px 9px;background:#23372f;display:inline-block}
.ok{color:#8ff1bb}.bad{color:#ffb3a0}.warn{color:#ffd59b}
.buttons{display:flex;flex-wrap:wrap;gap:8px;margin-top:12px}
button,a.action{cursor:pointer;border:1px solid #527765;background:#1b4538;color:#f0fff7;font:inherit;font-size:.9rem;font-weight:650;border-radius:8px;padding:10px 13px}
button:hover,a.action:hover{background:#276149}button:disabled{opacity:.55;cursor:wait}
pre{font:12px/1.6 ui-monospace,SFMono-Regular,Consolas,monospace;max-height:390px;overflow:auto;white-space:pre-wrap;overflow-wrap:anywhere;color:#bceccf;background:#081512;padding:13px;border-radius:9px;margin:12px 0 0}
ul{padding-left:20px;margin:10px 0 0}.state{border-radius:8px;padding:12px;background:#263629;margin-bottom:12px;font-size:.89rem}
.note{padding:12px;border-radius:9px;background:#1a302a;color:#c3dad0;font-size:.83rem;line-height:1.55;margin-top:12px}
footer{padding:26px 0;font-size:.78rem;color:#849d95}
a{color:#b3f2d3;text-decoration:underline}.tiny{font-size:.75rem;color:#9db5ab}

.term-form{display:flex;align-items:center;gap:9px;margin:13px 0 8px;flex-wrap:wrap}
.term-prompt{font:700 1.1rem Consolas,monospace;color:#8ff1bb}
.term-input{flex:1;min-width:160px;background:#081512;border:1px solid #436856;color:#e7fff3;
 border-radius:8px;padding:11px 12px;font:14px Consolas,ui-monospace,monospace}
.term-input:focus{outline:2px solid #71c49b;outline-offset:1px}
.term-log{min-height:120px;max-height:325px}
@media(max-width:720px){main{padding:12px}.g{grid-template-columns:repeat(2,minmax(0,1fr))}.pair{grid-template-columns:1fr}header{align-items:flex-start}h1{font-size:1.4rem}}
</style></head><body>
<main>
<header><div><div class="logo">FERNANDO LAB</div><h1>BOTIZIN / Central de diagnóstico</h1></div>
<div class="flag">PAINEL LOCAL · SEM POLLING</div></header>
<p class="description">Leitura apenas quando você abre a página ou aperta um botão. Este painel não instala firmware nem envia comandos à placa.</p>
<div class="state" id="state" role="status" aria-live="polite">Nenhum diagnóstico recebido.</div>
<div class="g" id="metrics">
<div class="metric"><div class="k">Firmware / slot</div><div class="v" id="firmware">—</div></div>
<div class="metric"><div class="k">Wi-Fi / HTTP</div><div class="v" id="network">—</div></div>
<div class="metric"><div class="k">OTA / rollback</div><div class="v" id="ota">—</div></div>
<div class="metric"><div class="k">RAM disponível</div><div class="v" id="memory">—</div></div>
</div>
<section class="pair">
<div class="panel"><h2>Visão geral</h2>
<p class="muted small">Snapshot solicitado, não uma medição permanente.</p>
<div id="overview">Aguardando primeira leitura.</div>
<div class="buttons"><button id="refresh" type="button">Consultar status agora</button>
<button id="copy" type="button" disabled>Copiar diagnóstico</button></div>
<div class="tiny" id="statusTime"></div>
</div>
<div class="panel"><h2>Eventos e falhas</h2>
<p class="muted small">Últimos 64 eventos na RAM, somente sob demanda.</p>
<div class="buttons"><button id="eventsBtn" type="button">Ler eventos agora</button>
<a class="action" href="/events" target="_blank" rel="noopener">Abrir texto</a></div>
<pre id="events">Nenhum evento carregado.</pre>
<div class="tiny" id="eventTime"></div>
</div>
</section>
<section class="panel" id="terminal-panel">
<h2>Terminal de diagnóstico · somente leitura</h2>
<p class="muted small">Comandos permitidos: <code>ajuda</code>, <code>status</code>, <code>eventos</code>, <code>boot</code>, <code>wifi</code>, <code>http</code>, <code>ota</code>, <code>memoria</code>, <code>mqtt</code>, <code>thingsboard</code>, <code>limpar</code>. Nenhum PowerShell é executado.</p>
<pre class="term-log" id="termOutput" role="log" aria-live="polite">BOTIZIN Console v1 | Somente consulta manual.
Digite ajuda para listar comandos.</pre>
<form class="term-form" id="termForm" autocomplete="off">
<label for="termInput" class="term-prompt">&gt;</label>
<input id="termInput" class="term-input" type="text" maxlength="32" autocomplete="off" autocapitalize="off" spellcheck="false" placeholder="Ex.: ota" aria-label="Comando de diagnóstico"/>
<button id="termRun" type="submit">Consultar</button></form>
<p class="tiny">O terminal consulta <code>/status</code> ou <code>/events</code> exclusivamente após Enter ou clique. Se o HTTP falhar, consulta indisponível; USB/MQTT seguem independentes.</p>
</section>
<section class="panel"><h2>Saídas alternativas quando o HTTP falhar</h2>
<p class="small muted">Se a página já está aberta e a leitura falha, o painel indica indisponibilidade. Se nem a página abrir, ela própria não poderá avisar.</p>
<ul class="small">
<li><b>USB serial</b> — eventos <code>EVT|...</code> a 115200 baud, mesmo sem Wi-Fi, se a placa estiver ligada.</li>
<li><b>MQTT existente</b> — resumo enviado pela S3 no ciclo habitual de ~90 segundos quando houver internet; o campo <code>http_age_s</code> só indica quando ela respondeu pela última vez, não é um teste externo de disponibilidade.</li>
<li><b>Último evento crítico</b> — guardado na NVS e recuperado no próximo boot, sem gravações contínuas.</li>
</ul>
<div class="note">Este terminal apenas lê diagnósticos. Botões que alteram Wi-Fi, OTA ou reiniciam ficam reservados para a próxima etapa, com autenticação.</div>
</section>
<footer>BOTIZIN 1.0 • Diagnóstico primeiro, comandos depois • Nenhum refresh automático</footer>
</main>
<script>
'use strict';
const $=id=>document.getElementById(id);
let latestStatus='';
let statusLoaded=false;
const get=(obj,key)=>obj[key]||'—';
function parseStatus(txt){const x={};for(const l of txt.split(/\r?\n/)){const i=l.indexOf(': ');if(i>0)x[l.slice(0,i)]=l.slice(i+2)}return x}
function setState(message,kind){const e=$('state');e.textContent=message;e.style.background=kind==='bad'?'#60302a':kind==='ok'?'#1a4a35':'#3c3827'}
async function read(endpoint){
 const ctrl=new AbortController();const timeout=setTimeout(()=>ctrl.abort(),9000);
 try{const r=await fetch(endpoint,{cache:'no-store',signal:ctrl.signal,redirect:'error'});if(!r.ok)throw new Error('HTTP '+r.status);return await r.text()}
 finally{clearTimeout(timeout)}
}
function renderStatus(txt){
 latestStatus=txt;statusLoaded=true;$('copy').disabled=false;
 const d=parseStatus(txt),first=txt.split(/\r?\n/)[0].replace('BOTIZIN CORE ','');
 const wifi=get(d,'WIFI'),server=get(d,'HTTP_SERVER_STARTED'),confirmed=get(d,'HTTP_CONFIRMED_BY_REQUEST');
 const state=get(d,'RUNNING_OTA_STATE');
 $('firmware').textContent=first+' / '+get(d,'RUNNING_PARTITION').split(' / ')[1];
 $('network').textContent=wifi+' · HTTP '+(server==='YES'?'iniciado':'não confirmado');
 $('ota').textContent=state+' · '+get(d,'INTERNET_OTA');
 const free=Number(d.DIAG_FREE);
 $('memory').textContent=Number.isFinite(free)?Math.round(free/1024)+' KB livres':'—';
 const fields=[
 ['Inicialização',get(d,'RESET_REASON')],
 ['Tempo ligado',get(d,'UPTIME_SECONDS')+' segundos'],
 ['HTTP atendido',confirmed],
 ['Última requisição',get(d,'HTTP_LAST_REQUEST_AGE_S')+' s atrás'],
 ['Gerenciador Wi-Fi',get(d,'WIFI_SETUP')],
 ['ThingsBoard',get(d,'TELEMETRY')],
 ['MQTT',get(d,'MQTT_LAST')],
 ['Eventos na RAM',get(d,'EVENTS_RAM')],
 ['Eventos perdidos',get(d,'EVENTS_OVERWRITTEN')]
 ];
 const div=$('overview');div.replaceChildren();
 for(const [key,val] of fields){const p=document.createElement('p');p.className='small';p.style.margin='7px 0';
   const label=document.createElement('b');label.textContent=key+': ';p.append(label,document.createTextNode(val));div.append(p)}
 $('statusTime').textContent='Última leitura manual ou inicial: '+new Date().toLocaleTimeString('pt-BR');
 setState('HTTP respondeu. Dados desta leitura; sem monitoramento contínuo.','ok');
}
async function loadStatus(){
 const btn=$('refresh');btn.disabled=true;setState('Consultando S3 uma única vez…','warn');
 try{renderStatus(await read('/status'))}
 catch(e){setState('HTTP INDISPONÍVEL nesta tentativa ('+e.message+'). Dados antigos podem estar desatualizados. Consulte USB ou MQTT.','bad')}
 finally{btn.disabled=false}
}
async function loadEvents(){
 const btn=$('eventsBtn');btn.disabled=true;$('events').textContent='Lendo eventos…';
 try{$('events').textContent=await read('/events');$('eventTime').textContent='Eventos lidos às '+new Date().toLocaleTimeString('pt-BR')}
 catch(e){$('events').textContent='EVENTOS INDISPONÍVEIS: '+e.message;setState('Não foi possível consultar /events. Confira USB/MQTT.','bad')}
 finally{btn.disabled=false}
}
$('refresh').addEventListener('click',loadStatus);
$('eventsBtn').addEventListener('click',loadEvents);
$('copy').addEventListener('click',async()=>{if(!statusLoaded)return;try{await navigator.clipboard.writeText(latestStatus);$('statusTime').textContent='Diagnóstico copiado.'}catch(e){$('statusTime').textContent='Cópia não permitida neste navegador. Use /status.'}});
// Command allowlist runs INSIDE browser. No eval, shell, hardware writes or control endpoints.
const allowed=['ajuda','status','eventos','boot','wifi','http','ota','memoria','mqtt','thingsboard','limpar'];
const termLog=$('termOutput');
function termWrite(line){
 termLog.textContent+='\n'+line;
 if(termLog.textContent.length>14000)termLog.textContent=termLog.textContent.slice(-12500);
 termLog.scrollTop=termLog.scrollHeight;
}
function termFields(d,fields){return fields.map(([label,key])=>label+': '+get(d,key)).join('\n')}
const views={
 boot:[['Firmware','BOTIZIN_VERSION'],['Reinício','RESET_REASON'],['Uptime (s)','UPTIME_SECONDS'],['Boot ID','BOOT_ID'],['Boot count','EVENTS_BOOT_COUNT']],
 wifi:[['Conexão STA','WIFI'],['IP','IP'],['Nome local','LOCAL_NAME'],['Configuração AP','WIFI_SETUP'],['Servidor HTTP iniciado','HTTP_SERVER_STARTED']],
 http:[['Servidor iniciado','HTTP_SERVER_STARTED'],['HTTP já respondeu','HTTP_CONFIRMED_BY_REQUEST'],['Total requisições','HTTP_REQUESTS_SINCE_BOOT'],['Última resposta (s atrás)','HTTP_LAST_REQUEST_AGE_S'],['Endereço local','LOCAL_NAME']],
 ota:[['Versão do manifesto','INTERNET_OTA'],['Estado firmware','RUNNING_OTA_STATE'],['Rodando','RUNNING_PARTITION'],['Próxima gravação','NEXT_UPDATE_PARTITION'],['Confirmação','OTA_CONFIRMATION_MODE'],['OTA disponível','OTA'],['Manual','OTA_MANUAL_STATUS']],
 memoria:[['Heap livre (bytes)','DIAG_FREE'],['Heap mínimo (bytes)','DIAG_MIN'],['Maior bloco (bytes)','DIAG_LARGEST'],['PSRAM livre (bytes)','DIAG_PSRAM'],['Espaço OTA livre (bytes)','DIAG_OTA_FREE']],
 mqtt:[['Última publicação','MQTT_LAST'],['Tentativas','MQTT_ATTEMPTS'],['Sucessos','MQTT_SUCCESSES']],
 thingsboard:[['Telemetria','TELEMETRY'],['Maior tempo de envio (us)','DIAG_TB_MAX_US']]
};
async function terminalCommand(command){
 const cmd=command.trim().toLowerCase().normalize('NFD').replace(/[\u0300-\u036f]/g,'');
 if(!cmd)return;
 termWrite('> '+cmd);
 if(!allowed.includes(cmd)){termWrite('Comando não permitido. Use ajuda.');return}
 if(cmd==='ajuda'){termWrite(allowed.join('  ·  ')+'\nOs comandos só leem dados; nada é executado no Windows ou na ESP32.');return}
 if(cmd==='limpar'){termLog.textContent='BOTIZIN Console v1 | Histórico local limpo.';return}
 const submit=$('termRun');submit.disabled=true;
 try{
   if(cmd==='eventos'){termWrite(await read('/events'));return}
   const response=await read('/status');
   if(cmd==='status'){termWrite(response.trim());return}
   const d=parseStatus(response);
   if(cmd==='boot')d.BOTIZIN_VERSION=response.split(/\r?\n/)[0].replace('BOTIZIN CORE ','');
   termWrite(termFields(d,views[cmd]));
 }catch(err){
   termWrite('ERRO: HTTP indisponível ('+err.message+'). Sem diagnóstico nesta rede. Verifique USB ou MQTT.');
 }finally{submit.disabled=false}
}
$('termForm').addEventListener('submit',e=>{
 e.preventDefault();
 const input=$('termInput'),cmd=input.value;input.value='';
 terminalCommand(cmd);
});
// No periodic terminal polls: network reads require a human command.
// Exactly one startup read of each resource. No interval, socket or background probes.
loadStatus();loadEvents();
</script>
</body></html>)BTZN";
