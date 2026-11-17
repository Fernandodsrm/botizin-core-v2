#pragma once
// Static flash-backed panel. No polling, timers or server-side dynamic page generation.
// /status and /events are read ONLY after the page loads once or a human clicks.
#include <pgmspace.h>
static const char kBotizinPanelHTML[] PROGMEM = R"BTZN(<!doctype html>
<html lang="pt-BR"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="color-scheme" content="dark"><title>BOTIZIN | Central da S3</title>
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
[hidden]{display:none!important}
.control-input{background:#081512;border:1px solid #436856;border-radius:8px;color:#f0fff7;padding:11px 12px;font:inherit;font-size:.9rem;width:100%;max-width:370px}
.control-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:12px}
.control-grid .panel{margin:0}.control-form{display:flex;flex-direction:column;gap:8px}
.control-form label{font-size:.82rem;color:#c3dad0}
.control-line{display:flex;gap:9px;align-items:center;flex-wrap:wrap}
@media(max-width:720px){.control-grid{grid-template-columns:1fr}}
@media(max-width:720px){main{padding:12px}.g{grid-template-columns:repeat(2,minmax(0,1fr))}.pair{grid-template-columns:1fr}header{align-items:flex-start}h1{font-size:1.4rem}}
.tabs{display:flex;gap:8px;flex-wrap:wrap;margin:20px 0}.tabs button[aria-selected="true"]{background:#91fac9;color:#08261c}.link-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(190px,1fr));gap:12px}.lamp{display:inline-block;width:10px;height:10px;border-radius:50%;background:#7b8b85;margin-right:7px}.lamp.ok{background:#65e8a6}.lamp.warn{background:#e5b35c}.lamp.bad{background:#ef8c81}.link-card{padding:16px;background:#102d27;border:1px solid #315046;border-radius:12px}.link-card h3{margin:0 0 12px;font-size:1rem}.link-card p{font-size:.85rem}.tabs button:focus-visible{outline:3px solid #91fac9}details summary{cursor:pointer;padding:8px;font-weight:650}
</style></head><body>
<main>
<header><div><div class="logo">FERNANDO LAB</div><h1>BOTIZIN / Central da S3</h1></div>
<div class="flag">WIFI 2.2 · CORE 0.0.28 CANDIDATA</div></header>
<p class="description">CORE 0.0.28 candidata: Wi-Fi 2.2 com até 5 redes lembradas e recuperação orientada. MQTT e OTA preservados. ThingsBoard e RPC legados desativados nesta candidata.</p>
<div class="state" id="state" role="status" aria-live="polite">Nenhum diagnóstico recebido.</div>
<div class="g" id="metrics">
<div class="metric"><div class="k">Firmware / slot</div><div class="v" id="firmware">—</div></div>
<div class="metric"><div class="k">Wi-Fi / HTTP</div><div class="v" id="network">—</div></div>
<div class="metric"><div class="k">OTA / rollback</div><div class="v" id="ota">—</div></div>
<div class="metric"><div class="k">RAM disponível</div><div class="v" id="memory">—</div></div>
</div>

<nav class="tabs" role="tablist" aria-label="Áreas do painel">
<button role="tab" id="tab-home" aria-controls="page-home" aria-selected="true" data-page="home">Visão geral</button>
<button role="tab" id="tab-links" aria-controls="page-links" aria-selected="false" data-page="links">Conexões</button>
<button role="tab" id="tab-wifi" aria-controls="page-wifi" aria-selected="false" data-page="wifi">Wi-Fi</button>
<button role="tab" id="tab-resources" aria-controls="page-resources" aria-selected="false" data-page="resources">Recursos</button>
<button role="tab" id="tab-events" aria-controls="page-events" aria-selected="false" data-page="events">Eventos</button>
<button role="tab" id="tab-settings" aria-controls="page-settings" aria-selected="false" data-page="settings">Configurações</button>
<button role="tab" id="tab-diag" aria-controls="page-diag" aria-selected="false" data-page="diag">Diagnóstico</button>
</nav>
<div id="page-home" role="tabpanel" aria-labelledby="tab-home"></div>
<div id="page-links" role="tabpanel" aria-labelledby="tab-links" hidden>
<section class="panel"><h2>Quem se comunica com quem</h2>
<p class="small muted">Indicadores da última consulta à S3. Verde indica um resultado registrado, não uma conexão continuamente monitorada.</p>
<div class="link-grid">
<div class="link-card"><h3>S3 → Wi-Fi</h3><p id="link-wifi">Não verificada</p></div>
<div class="link-card"><h3>Navegador → S3</h3><p id="link-http">Não verificada</p></div>
<div class="link-card"><h3>S3 → MQTT</h3><p id="link-mqtt">Não verificada</p></div>
<div class="link-card"><h3>S3 ↔ WROOM</h3><p id="link-peer">Não verificada</p><a href="http://botizin-wroom.local/status" target="_blank" rel="noopener">Abrir status da WROOM</a></div>
<div class="link-card"><h3>GPT → ponte → S3</h3><p>Não verificada por este painel. Teste disponível na conversa pelo plugin.</p></div>
<div class="link-card"><h3>Ponte → A22</h3><p>Integração pendente. Nenhum comando disponível.</p></div>
</div><p class="tiny" id="links-time">Nenhuma leitura.</p>
<button type="button" id="links-refresh">Atualizar conexões da S3</button>
</section></div>
<div id="page-wifi" role="tabpanel" aria-labelledby="tab-wifi" hidden>
<section class="panel">
<h2>Wi-Fi / Conexão atual</h2>
<p class="muted small">Leitura pontual, sem varredura de redes nem teste ativo. Nenhuma senha é exibida.</p>
<div class="link-grid">
<div class="link-card"><h3>Rede conectada</h3><p id="wf-ssid">Não consultado</p></div>
<div class="link-card"><h3>IP atual da S3</h3><p id="wf-ip">—</p></div>
<div class="link-card"><h3>Qualidade do sinal</h3><p id="wf-quality">—</p></div>
<div class="link-card"><h3>Endereço local (mDNS)</h3><p id="wf-mdns">—</p></div>
</div>
<p class="note" id="wf-ap">Recuperação Wi-Fi preparada.</p>
<details class="note"><summary>Detalhes técnicos: canal, RSSI, gateway, DNS</summary>
<div class="link-grid">
<div><b>RSSI:</b> <span id="wf-rssi">—</span></div>
<div><b>Canal:</b> <span id="wf-channel">—</span></div>
<div><b>Gateway:</b> <span id="wf-gateway">—</span></div>
<div><b>DNS:</b> <span id="wf-dns">—</span></div>
</div><p class="tiny">Gateway e DNS podem ter o mesmo IP. mDNS iniciado não garante que Windows ou hotspot encontrem o nome .local.</p></details>
<div class="buttons"><button id="wifi-read" type="button">Consultar Wi-Fi agora</button></div>
<p class="tiny" role="status" aria-live="polite" id="wf-feedback">Aguardando consulta manual.</p>
<div class="panel" style="margin-top:14px">
<h2>Wi-Fi 2.2 · Teste simples, resultado automático</h2>
<p class="muted small">Escolha a rede e clique em <b>Testar e voltar</b>. A S3 conecta, verifica e retorna à anterior. O resultado aparece aqui, sem trocar o Wi-Fi do notebook nem copiar linhas de /status.</p>
<p class="tiny" id="wf-admin-feedback" role="status">Acesso liberado pela senha única do painel. Esta aba não pede outra autenticação.</p>
<div class="buttons"><button id="wf-scan" type="button">Procurar redes 2,4 GHz</button></div>
<label for="wf-network-choice" class="small muted">Redes encontradas (opcional)</label>
<select id="wf-network-choice" class="control-input" style="max-width:100%"><option value="">Selecione ou digite o nome abaixo</option></select>
<form id="wf-change-form" class="control-form" autocomplete="off" style="margin-top:12px">
<label for="wf-target-ssid">Nome da rede (SSID)</label>
<input class="control-input" id="wf-target-ssid" maxlength="32" required autocomplete="off" placeholder="Nome da rede 2,4 GHz"/>
<label for="wf-target-pass">Senha da nova rede</label>
<input class="control-input" id="wf-target-pass" type="password" maxlength="63" autocomplete="new-password" placeholder="Senha WPA2 (ou vazio para rede aberta)"/>
<button type="submit" id="wf-test">Testar e voltar automaticamente</button>
</form>
<div class="buttons"><button id="wf-confirm" type="button" disabled>Salvar rede testada para próximo reinício</button></div>
<div class="state" id="wf-test-result" role="status" aria-live="polite">Nenhum teste iniciado. O resultado aparecerá automaticamente depois de testar.</div>
<div id="wf-tested-link" class="note" hidden><b>Endereço recebido pela S3 na rede testada:</b>
<input class="control-input" id="wf-tested-address" type="text" readonly/>
<div class="buttons"><button id="wf-copy" type="button">Copiar endereço</button>
<a id="wf-open" class="action" target="_blank" rel="noopener">Abrir painel pelo IP</a></div>
<p class="tiny">Antes de abrir, conecte o notebook ou celular à mesma rede testada. Este IP pode mudar futuramente.</p></div>
<p class="note">Durante o teste, este painel pode ficar temporariamente sem acesso: a S3 tem uma conexão Wi-Fi STA. <b>É esperado.</b> A S3 volta à rede de origem e o painel consulta o resultado por até 90 segundos. Só o botão separado <b>Salvar rede testada</b> grava a nova rede para uso após reiniciar.</p>
<div class="panel" style="margin-top:14px"><h2>Redes conhecidas (até 5)</h2>
<p class="muted small">A S3 não muda de rede por sinal mais forte. Só tenta outra rede lembrada se a atual cair. Nenhuma senha é exibida aqui.</p>
<div class="buttons"><button id="wf-known-refresh" type="button">Atualizar redes lembradas</button></div>
<div id="wf-known-list" class="note" role="status">Ainda não consultado.</div></div>
<details class="note"><summary>Guia de recuperação — se não conseguir abrir a S3</summary>
<p><b>Normal:</b> use http://botizin-s3.local/ na mesma rede. Se o nome falhar, tente o IP mostrado no teste de Wi-Fi.</p>
<p><b>Sem rede conhecida:</b> a S3 abre uma rede temporária BOTIZIN-S3-XXXX após cerca de 35 segundos sem Wi-Fi. Conecte-se a ela e abra http://192.168.4.1/.</p>
<p><b>Após configurar:</b> a página de recuperação informa o IP da rede nova, com opção de copiar. Conecte-se à nova rede antes de abrir o endereço.</p>
<p><b>Se está conectada mas perdeu o IP:</b> consulte os clientes do roteador ou recupere via USB. O portal não liga apenas porque mDNS deixou de funcionar.</p>
<p><b>Senha da rede de emergência:</b> gerada pela S3 e mostrada na USB, deve ser guardada em local seguro.</p>
</details>
</section></div>
<div id="page-resources" role="tabpanel" aria-labelledby="tab-resources" hidden>
<section class="panel">
<h2>Recursos e limites / S3</h2>
<p class="muted small">Snapshot da S3. Não mede custo individual de módulo; para isso serão necessárias compilações e medições comparativas.</p>
<div class="link-grid">
<div class="link-card"><h3>RAM interna livre</h3><p id="rs-free">—</p></div>
<div class="link-card"><h3>Menor RAM livre</h3><p id="rs-min">—</p></div>
<div class="link-card"><h3>Maior bloco contínuo</h3><p id="rs-largest">—</p></div>
<div class="link-card"><h3>PSRAM livre</h3><p id="rs-psram">—</p></div>
<div class="link-card"><h3>Espaço OTA estimado</h3><p id="rs-ota">—</p></div>
<div class="link-card"><h3>Firmware estimado</h3><p id="rs-firmware">—</p></div>
<div class="link-card"><h3>Maior intervalo de loop</h3><p id="rs-loop">—</p></div>
<div class="link-card"><h3>Idade da amostra</h3><p id="rs-sample-age">—</p></div>
</div>
<div class="buttons"><button id="resources-read" type="button">Medir recursos agora</button></div>
<p class="tiny" role="status" aria-live="polite" id="rs-feedback">Aguardando consulta manual. Nenhuma coleta periódica será ativada.</p>
</section></div>
<div id="page-events" role="tabpanel" aria-labelledby="tab-events" hidden></div>
<div id="page-settings" role="tabpanel" aria-labelledby="tab-settings" hidden></div>
<div id="page-diag" role="tabpanel" aria-labelledby="tab-diag" hidden></div>
<section class="pair" id="summary-panels">
<div class="panel"><h2>Visão geral</h2>
<p class="muted small">Snapshot solicitado, não uma medição permanente.</p>
<div id="overview">Aguardando primeira leitura.</div>
<div class="buttons"><button id="refresh" type="button">Consultar status agora</button>
<button id="copy" type="button" disabled>Copiar diagnóstico</button></div>
<div class="tiny" id="statusTime"></div>
</div>
<div class="panel" id="events-panel"><h2>Eventos e falhas</h2>
<p class="muted small">Últimos 64 eventos na RAM, somente sob demanda.</p>
<div class="buttons"><button id="eventsBtn" type="button">Ler eventos agora</button>
<a class="action" href="/events" target="_blank" rel="noopener">Abrir texto</a></div>
<pre id="events">Nenhum evento carregado.</pre>
<div class="tiny" id="eventTime"></div>
</div>
</section>
<section class="panel" id="terminal-panel">
<h2>Terminal de diagnóstico · somente leitura</h2>
<p class="muted small">Comandos permitidos: <code>ajuda</code>, <code>status</code>, <code>eventos</code>, <code>boot</code>, <code>wifi</code>, <code>http</code>, <code>ota</code>, <code>memoria</code>, <code>mqtt</code>, <code>limpar</code>. Nenhum PowerShell é executado.</p>
<pre class="term-log" id="termOutput" role="log" aria-live="polite">BOTIZIN Console v1 | Somente consulta manual.
Digite ajuda para listar comandos.</pre>
<form class="term-form" id="termForm" autocomplete="off">
<label for="termInput" class="term-prompt">&gt;</label>
<input id="termInput" class="term-input" type="text" maxlength="32" autocomplete="off" autocapitalize="off" spellcheck="false" placeholder="Ex.: ota" aria-label="Comando de diagnóstico"/>
<button id="termRun" type="submit">Consultar</button></form>
<p class="tiny">O terminal consulta <code>/status</code> ou <code>/events</code> exclusivamente após Enter ou clique. Se o HTTP falhar, consulta indisponível; USB/MQTT seguem independentes.</p>
</section>
<section class="panel" id="controls-panel">
<h2>Central de comandos · autenticada</h2>
<p class="muted small">Ações somente por clique, na rede local. Use a senha individual <b>BOTIZIN-S3-SETUP</b> impressa no USB, criada pela própria S3. Nenhuma senha é enviada ao GitHub.</p>
<div class="control-line" id="controlLoginBox" hidden>
<input id="controlPassword" type="password" class="control-input" maxlength="32" autocomplete="off" placeholder="Senha da configuração da S3" aria-label="Senha do painel"/>
<button id="controlLogin" type="button">Desbloquear comandos</button>
</div>
<div id="controlAuthStatus" class="tiny" role="status">Sessão ativa. Uma senha no início, 10 minutos de inatividade para expirar.</div>
<div id="controlActions"><p class="note">IMAGEM CANDIDATA 0.0.27: verificação e instalação OTA existentes foram restauradas. O manifesto de produção continua em 0.0.25; não publicar este firmware automaticamente antes de testar.</p>
<div class="buttons">
<button id="controlConnections" type="button">Verificar conexões</button>
<button id="controlCheckOTA" type="button">Consultar OTA agora</button>
<button id="controlOTAResult" type="button">Resultado da consulta OTA</button>
<button id="controlInstall" type="button" disabled>Instalar atualização confirmada</button>
<button id="controlCancel" type="button">Cancelar consulta OTA</button>
</div>
<div class="control-grid" style="margin-top:13px">
<div class="panel">
<h2>Wi-Fi 2.0</h2>
<p class="muted small">O novo teste verifica a rede, retorna sozinho e mostra sucesso ou falha no painel. Salvar é uma ação separada.</p>
<button id="settingsGotoWifi" type="button">Abrir Wi-Fi 2.0</button>
</div>
<div class="panel"><h2>Sistema</h2>
<p class="muted small">Reinício solicitado, nunca automático pelo painel.</p>
<button id="controlReboot" type="button">Reiniciar a S3</button>
<p class="tiny">Após reiniciar, entre com a senha administrativa novamente. O OTA mantém o rollback original do Arduino.</p>
<button id="controlLogout" type="button">Sair do painel</button>
</div>
</div>
<pre id="controlOutput" role="log" aria-live="polite">Comandos prontos. Somente ações iniciadas por você.</pre>
</div>
<div class="note">Segurança: a conexão local HTTP não usa HTTPS. Use apenas em uma rede Wi-Fi confiável, sem redirecionar portas do roteador. Comandos antigos de manutenção possuem rotas próprias e serão auditados separadamente.</div>
</section>
<section class="panel" id="recovery-panel"><h2>Saídas alternativas quando o HTTP falhar</h2>
<p class="small muted">Se a página já está aberta e a leitura falha, o painel indica indisponibilidade. Se nem a página abrir, ela própria não poderá avisar.</p>
<ul class="small">
<li><b>USB serial</b> — eventos <code>EVT|...</code> a 115200 baud, mesmo sem Wi-Fi, se a placa estiver ligada.</li>
<li><b>MQTT existente</b> — resumo enviado pela S3 no ciclo habitual de ~90 segundos quando houver internet; o campo <code>http_age_s</code> só indica quando ela respondeu pela última vez, não é um teste externo de disponibilidade.</li>
<li><b>Último evento crítico</b> — guardado na NVS e recuperado no próximo boot, sem gravações contínuas.</li>
</ul>
<div class="note">O terminal lê diagnósticos. Os controles de Wi-Fi, OTA e reinício ficam na aba Configurações e exigem autenticação.</div>
</section>
<footer>BOTIZIN • Central local da S3 • Dados atualizados sob demanda</footer>
</main>
<script>
'use strict';
const $=id=>document.getElementById(id);

// Layout changes preserve the existing control handlers and authentication.
$('page-home').append($('summary-panels'));
$('page-events').append($('events-panel'));
$('summary-panels').style.gridTemplateColumns='1fr';
$('page-settings').append($('controls-panel'));
$('page-diag').append($('terminal-panel'),$('recovery-panel'));
// Wi-Fi and Resources tabs deliberately do not start background polling.
document.querySelectorAll('[data-page]').forEach(button=>button.addEventListener('click',()=>{
 document.querySelectorAll('[data-page]').forEach(b=>{
  const active=b===button;b.setAttribute('aria-selected',String(active));
  $('page-'+b.dataset.page).hidden=!active;
 });
}));
function linkState(id,label,kind){
 const el=$(id),dot=document.createElement('span');
 dot.className='lamp '+kind;dot.setAttribute('aria-hidden','true');
 el.replaceChildren(dot,document.createTextNode(label));
}
$('links-refresh').addEventListener('click',loadStatus);
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
function showText(id,value){$(id).textContent=String(value)}
async function loadWifiReadOnly(){
  const btn=$('wifi-read');btn.disabled=true;
  showText('wf-feedback','Consultando Wi-Fi local…');
  try {
    const x=JSON.parse(await read('/read/wifi'));
    showText('wf-ssid',x.ssid||'Sem nome');
    showText('wf-ip',x.ip||'—');
    showText('wf-rssi',Number.isFinite(x.rssi_dbm)?x.rssi_dbm+' dBm':'—');
    const rssi=Number(x.rssi_dbm);
    showText('wf-quality',Number.isFinite(rssi)?
      (rssi>=-60?'Bom':rssi>=-75?'Regular':'Fraco')+' ('+rssi+' dBm)':'—');
    showText('wf-channel',Number.isFinite(x.channel)?x.channel:'—');
    showText('wf-gateway',x.gateway||'—');
    showText('wf-dns',x.dns||'—');
    showText('wf-mdns',x.mdns_ready?'botizin-s3.local (anunciado)':'mDNS não iniciado');
    showText('wf-ap',x.recovery_ap_active?'Recuperação ativa: use a rede BOTIZIN-S3 e abra http://192.168.4.1/':
'Recuperação preparada. A rede de emergência fica desligada quando não é necessária.');
    showText('wf-feedback','Lido às '+new Date().toLocaleTimeString('pt-BR')+'. Valores desta consulta, não monitoramento contínuo.');
  }catch(e){
    showText('wf-feedback','Consulta Wi-Fi indisponível: '+e.message+'. Dados anteriores podem estar desatualizados.');
  }finally{btn.disabled=false}
}
async function loadResourceReadOnly(){
  const btn=$('resources-read');btn.disabled=true;
  showText('rs-feedback','Consultando memória…');
  try {
    const d=parseStatus(await read('/status'));
    const bytes=(key)=>{const n=Number(d[key]);return Number.isFinite(n)&&n>=0?n.toLocaleString('pt-BR')+' B':'—'};
    for(const [id,key] of [
      ['rs-free','DIAG_FREE'],['rs-min','DIAG_MIN'],['rs-largest','DIAG_LARGEST'],
      ['rs-psram','DIAG_PSRAM'],['rs-ota','DIAG_OTA_FREE']
    ])showText(id,bytes(key));
    const remaining=Number(d.DIAG_OTA_FREE);
    const slot=3145728; // CORE 0.0.25: OTA slot capacity, NOT total flash size.
    showText('rs-firmware',Number.isFinite(remaining)&&remaining>=0&&remaining<=slot?
      (slot-remaining).toLocaleString('pt-BR')+' B (estimado)':'—');
    const gap=Number(d.DIAG_LOOP_MAX_US);
    showText('rs-loop',Number.isFinite(gap)&&gap>=0?(gap/1000).toLocaleString('pt-BR')+' ms':'—');
    showText('rs-sample-age',d.DIAG_SAMPLE_AGE?d.DIAG_SAMPLE_AGE+' s':'—');
    showText('rs-feedback','Lido às '+new Date().toLocaleTimeString('pt-BR')+'. São contadores e amostras existentes no firmware.');
  }catch(e){
    showText('rs-feedback','Consulta de recursos indisponível: '+e.message+'. Dados anteriores podem estar desatualizados.');
  }finally{btn.disabled=false}
}
$('wifi-read').addEventListener('click',loadWifiReadOnly);
$('resources-read').addEventListener('click',loadResourceReadOnly);

function renderStatus(txt){
 latestStatus=txt;statusLoaded=true;$('copy').disabled=false;
 const links=parseStatus(txt);
 linkState('link-wifi',links.WIFI==='OK'?'Wi-Fi conectado nesta leitura':'Wi-Fi: '+get(links,'WIFI'),links.WIFI==='OK'?'ok':'warn');
 linkState('link-http','S3 respondeu a esta consulta','ok');
 linkState('link-mqtt','Último resultado: '+get(links,'MQTT_LAST'),links.MQTT_LAST==='PUBLISHED'?'ok':'warn');
 linkState('link-peer',links.PEER_PAIRED==='YES'?'Pareamento salvo; comunicação atual não testada':'Pareamento: '+get(links,'PEER_PAIRED'),'warn');
 $('links-time').textContent='Leitura: '+new Date().toLocaleTimeString('pt-BR');
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
 ['Telemetria legada', 'Desativada'],
 ['MQTT',get(d,'MQTT_LAST')],
 ['Eventos na RAM',get(d,'EVENTS_RAM')],
 ['Eventos perdidos',get(d,'EVENTS_OVERWRITTEN')]
 ];
 const div=$('overview');div.replaceChildren();
 for(const [key,val] of fields){const p=document.createElement('p');p.className='small';p.style.margin='7px 0';
   const label=document.createElement('b');label.textContent=key+': ';p.append(label,document.createTextNode(val));div.append(p)}
 $('statusTime').textContent='Última leitura manual ou inicial: '+new Date().toLocaleTimeString('pt-BR');
 setState('HTTP respondeu. Dados desta leitura; sem monitoramento contínuo.','ok');
 renderWifiTrial(d);
 if(['QUEUED','CONNECTING','OBSERVING','RETURNING'].includes(d.WIFI_TRIAL_STATE))startWifiWatch();
}
async function loadStatus(){
 const btn=$('refresh');btn.disabled=true;setState('Consultando S3 uma única vez…','warn');
 try{renderStatus(await read('/status'))}
 catch(e){linkState('link-http','Falha na última consulta','bad');setState('HTTP INDISPONÍVEL nesta tentativa ('+e.message+'). Dados antigos podem estar desatualizados. Consulte USB ou MQTT.','bad')}
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
const allowed=['ajuda','status','eventos','boot','wifi','http','ota','memoria','mqtt','limpar'];
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
// Protected controls: all network calls below require a deliberate click/submit.
// Session is volatile in this browser tab; never localStorage/cookies.
let controlSession='cookie';
let currentCandidate=null;
function controlOutput(message){const e=$('controlOutput');e.textContent=message; e.scrollTop=e.scrollHeight}
function clearControlSession(message){
 controlSession='';currentCandidate=null;
 $('controlInstall').disabled=true;
 $('controlAuthStatus').textContent=message||'Sessão expirada. Entre novamente.';
 window.location.replace('/');
}
async function controlPost(path,fields={},authentication=true){
 const params=new URLSearchParams(fields);
 const headers={'Content-Type':'application/x-www-form-urlencoded'};
 if(authentication && /^[0-9a-f]{32}$/.test(controlSession))headers['X-Botizin-Session']=controlSession;
 const ctrl=new AbortController(),timeout=setTimeout(()=>ctrl.abort(),13000);
 try {
  const r=await fetch(path,{method:'POST',headers,body:params.toString(),cache:'no-store',credentials:'same-origin',
                            redirect:'error',signal:ctrl.signal});
  const body=await r.text();
  if(!r.ok)throw new Error('HTTP '+r.status+': '+body);
  return body.trim();
 } finally{clearTimeout(timeout)}
}
async function runControl(path,fields={}){
 if(!controlSession){clearControlSession('Faça login novamente.');return null}
 try{const result=await controlPost(path,fields);controlOutput(result);return result}
 catch(e){controlOutput('ERRO: '+e.message);
   if(String(e.message).includes('AUTH_REQUIRED_OR_EXPIRED'))clearControlSession('Sessão expirada. Autentique novamente.');
   return null}
}
$('controlLogout').addEventListener('click',async()=>{
 if(controlSession)try{await controlPost('/control/logout')}catch(e){}
 window.location.replace('/');
});
$('controlConnections').addEventListener('click',()=>runControl('/control/connections'));
$('controlCheckOTA').addEventListener('click',async()=>{
 currentCandidate=null;$('controlInstall').disabled=true;
 const res=await runControl('/control/check-ota');
 if(res)controlOutput(res+'\nA S3 consultará o GitHub uma vez. Clique em "Resultado da consulta OTA" depois.');
});
$('controlOTAResult').addEventListener('click',async()=>{
 const r=await runControl('/control/ota-result');
 if(!r)return;
 if(r.startsWith('AVAILABLE|')){
  const p=r.split('|');
  if(p.length===6 && /^[0-9a-f]{64}$/.test(p[3])){
   currentCandidate={candidate_id:p[1],target_version:p[2],sha256:p[3],boot_id:p[4]};
   $('controlInstall').disabled=false;
   controlOutput('ATUALIZAÇÃO DISPONÍVEL\nVersão: '+p[2]+'\nTamanho: '+p[5]+' bytes\nSHA-256: '+p[3]+
     '\nPara instalar, clique no botão de confirmação. Nenhuma instalação começou.');
  }
 }else{currentCandidate=null;$('controlInstall').disabled=true}
});
$('controlInstall').addEventListener('click',async()=>{
 if(!currentCandidate)return;
 const c=currentCandidate;
 if(!window.confirm('Instalar firmware '+c.target_version+' e reiniciar a S3? A instalação só será iniciada após confirmar.'))return;
 currentCandidate=null;$('controlInstall').disabled=true;
 await runControl('/control/install-ota',c);
});
$('controlCancel').addEventListener('click',async()=>{
 currentCandidate=null;$('controlInstall').disabled=true;
 await runControl('/control/cancel-ota');
});
// One server-side login; every action uses the same HttpOnly cookie.
// Retries exist only while a test explicitly started by the user is active.
let wifiWatchId=0;
function wifiFeedback(message){showText('wf-test-result',message);$('wf-test-result').style.whiteSpace='pre-line'}
function renderWifiTrial(d){
 const st=d.WIFI_TRIAL_STATE;
 if(!st||st==='IDLE')return;
 const target=d.WIFI_TRIAL_TARGET||'rede escolhida';
 const original=d.WIFI_TRIAL_RETURNED_WIFI||'rede anterior';
 let msg='';
 if(st==='TEST_OK'){
   msg='✅ TESTE APROVADO!\nConectou a '+target+
   (d.WIFI_TRIAL_TARGET_IP?' (IP: '+d.WIFI_TRIAL_TARGET_IP+')':'')+
   '.\nRetornou automaticamente a '+original+'.\nA nova rede NÃO foi salva.';
 }else if(st==='TEST_FAILED'){
   msg='❌ NÃO CONECTOU à rede '+target+'.\nMotivo: '+(d.WIFI_TRIAL_REASON||'desconhecido')+
   '.\nConexão original recuperada: '+original+'.';
 }else if(st==='RECOVERY_FAILED'){
   msg='⚠️ A S3 NÃO CONFIRMOU o retorno à rede antiga. Use o portal de recuperação se necessário.';
 }else if(st==='SAVED_FOR_NEXT_RESTART'){
   msg='✅ REDE SALVA para o próximo reinício.\nA S3 ainda está conectada à rede atual.';
 }else{
   const stage={QUEUED:'Preparando o teste',CONNECTING:'Conectando à rede nova',
     OBSERVING:'Conectou! Verificando estabilidade',RETURNING:'Voltando à rede anterior'};
   msg='⏳ '+(stage[st]||st)+'…\nAguarde: o painel pode perder acesso temporariamente.';
 }
 wifiFeedback(msg);
 $('wf-confirm').disabled=st!=='TEST_OK'||d.WIFI_TRIAL_CAN_SAVE!=='YES';
 const ip=d.WIFI_TRIAL_TARGET_IP||'';
 const validIp=/^\d{1,3}(\.\d{1,3}){3}$/.test(ip);
 $('wf-tested-link').hidden=!validIp;
 if(validIp){const url='http://'+ip+'/';$('wf-tested-address').value=url;$('wf-open').href=url;}
}
async function wifiCommand(path,fields={},quiet=false){
 try{return await controlPost(path,fields)}
 catch(e){
   if(String(e.message).includes('AUTH_REQUIRED_OR_EXPIRED')){
     clearControlSession('Sessão expirada. Entre novamente.');
     return null;
   }
   if(!quiet)wifiFeedback('Não foi possível realizar a ação: '+e.message);
   return null;
 }
}
function startWifiWatch(){
 if(wifiWatchId)return;
 const watch=++wifiWatchId, started=Date.now();
 $('wf-test').disabled=true;
 async function tick(){
   if(watch!==wifiWatchId)return;
   const result=await wifiCommand('/control/wifi-state',{},true);
   if(watch!==wifiWatchId)return;
   if(result){
     const d=parseStatus(result);
     renderWifiTrial(d);
     if(['TEST_OK','TEST_FAILED','RECOVERY_FAILED','SAVED_FOR_NEXT_RESTART'].includes(d.WIFI_TRIAL_STATE)){
       wifiWatchId=0;$('wf-test').disabled=false;
       if(d.WIFI_TRIAL_STATE!=='RECOVERY_FAILED')loadWifiReadOnly();
       return;
     }
   }else{
     wifiFeedback('⏳ A S3 está trocando de rede. Sem acesso temporário. O painel tentará novamente sozinho…');
   }
   if(Date.now()-started<90000)window.setTimeout(tick,2400);
   else{
     wifiWatchId=0;$('wf-test').disabled=false;
     wifiFeedback('⚠️ Não foi possível confirmar o resultado em 90 segundos. Confira a rede original e abra novamente o painel.');
   }
 }
 tick();
}
$('wf-scan').addEventListener('click',async()=>{
  const btn=$('wf-scan');btn.disabled=true;
  wifiFeedback('Procurando redes 2,4 GHz nesta solicitação…');
  try{
    const txt=await wifiCommand('/control/wifi-scan');
    if(!txt)return;
    const list=JSON.parse(txt).networks||[];
    const select=$('wf-network-choice');
    select.replaceChildren(new Option('Selecione ou digite o nome abaixo',''));
    for(const item of list){
      if(typeof item.ssid!=='string')continue;
      select.add(new Option(item.ssid+' · '+item.rssi_dbm+' dBm · canal '+item.channel,item.ssid));
    }
    wifiFeedback(list.length+' redes encontradas. Selecione uma ou digite o SSID manualmente.');
  }catch(e){wifiFeedback('Não foi possível ler o resultado: '+e.message)}
  finally{btn.disabled=false}
});
$('wf-network-choice').addEventListener('change',()=>{
  if($('wf-network-choice').value) $('wf-target-ssid').value=$('wf-network-choice').value;
});
$('wf-change-form').addEventListener('submit',async e=>{
 e.preventDefault();
 if(wifiWatchId)return;
 const ssid=$('wf-target-ssid').value.trim(),password=$('wf-target-pass').value;
 if(!ssid||(password.length>0&&password.length<8)){wifiFeedback('Nome da rede ou senha inválidos.');return}
 $('wf-test').disabled=true;$('wf-confirm').disabled=true;
 wifiFeedback('Iniciando teste na rede '+ssid+'…');
 const result=await wifiCommand('/control/wifi-test',{ssid,password});
 $('wf-target-pass').value='';
 if(result){
   wifiFeedback('⏳ Testando '+ssid+'. A S3 voltará sozinha à rede anterior e mostrará o resultado.');
   startWifiWatch();
 }else $('wf-test').disabled=false;
});
$('wf-confirm').addEventListener('click',async()=>{
 if($('wf-confirm').disabled)return;
 if(!window.confirm('Salvar a rede testada para o PRÓXIMO REINÍCIO? A S3 permanece na rede atual até reiniciar.'))return;
 const result=await wifiCommand('/control/wifi-confirm');
 $('wf-confirm').disabled=true;
 if(result){
  wifiFeedback('✅ Rede salva entre as redes lembradas. A S3 continua na rede atual até reiniciar.');
  refreshKnownNetworks();
 }
});
$('settingsGotoWifi').addEventListener('click',()=>{$('tab-wifi').click()});
$('controlReboot').addEventListener('click',async()=>{
 if(!window.confirm('Reiniciar a S3 agora?'))return;
 const result=await runControl('/control/reboot');
 if(result)clearControlSession('Reinício solicitado. A conexão será interrompida.');
});
// No polling while idle; bounded retries only after a user-initiated Wi-Fi test.
loadStatus();loadEvents();
</script>
</body></html>)BTZN";
