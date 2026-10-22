#pragma once
// BOTIZIN LAB web-voice proof of concept (NOT XiaoZhi AI).
// No device commands, no polling, no external JavaScript dependencies.
#include <pgmspace.h>
static const char kBotizinLabHTML[] PROGMEM = R"BTZNLAB(<!doctype html>
<html lang="pt-BR"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>BOTIZIN LAB | Voz e rosto</title>
<style>
:root{color-scheme:dark;font-family:system-ui,-apple-system,Segoe UI,sans-serif;background:#0b1615;color:#eafff4}
*{box-sizing:border-box}body{margin:0;padding:16px}main{max-width:650px;margin:0 auto}
header{display:flex;justify-content:space-between;align-items:center;gap:12px}
h1{font-size:1.3rem;margin:5px 0}h2{font-size:1rem}
a{color:#8ce3bd}small,.muted{color:#a1bdb0}
.card{background:#142925;border:1px solid #34594d;border-radius:17px;padding:18px;margin-top:14px}
.face{position:relative;width:min(100%,300px);height:180px;margin:14px auto;background:#061411;border:2px solid #2c6653;border-radius:25px;display:flex;justify-content:space-evenly;align-items:center}
.eye{width:72px;height:85px;background:#92ffd0;border-radius:35%;animation:blink 6.1s infinite}
.face.listening .eye{background:#ffd18b}.face.speaking .eye{animation:talk 0.8s infinite alternate}
.face.thinking .eye{animation:thinking 1s infinite alternate}
@keyframes blink{0%,94%,99%,100%{transform:scaleY(1)}96%{transform:scaleY(.1)}}
@keyframes talk{from{transform:scaleY(.73)}to{transform:scaleY(1)}}
@keyframes thinking{from{transform:translateX(-7px)}to{transform:translateX(7px)}}
.state{text-align:center;font-size:.9rem;color:#b6e8d2}
.actions{display:flex;gap:9px;flex-wrap:wrap}button{background:#52bc8e;border:0;border-radius:10px;color:#08221a;padding:12px 15px;font-weight:750;font:inherit;cursor:pointer}
button:disabled{opacity:.4;cursor:not-allowed}.secondary{background:#294e42;color:#d1fbea}
input{font:inherit;width:100%;min-width:0;border-radius:10px;padding:12px;background:#071c17;color:#f4fff8;border:1px solid #457766}
form{display:flex;gap:9px;align-items:center}.log{white-space:pre-wrap;overflow-wrap:anywhere;min-height:85px;font-size:.88rem;color:#d1fbea}
.warn{color:#ffd18b}#voiceInfo{font-size:.83rem;line-height:1.5}
</style></head><body><main>
<header><div><small>FERNANDO LAB • PRÉVIA EXPERIMENTAL</small><h1>BOTIZIN • LAB 0.0.26</h1></div>
<a href="/panel">Painel técnico</a></header>
<section class="card"><div id="face" class="face"><div class="eye"></div><div class="eye"></div></div>
<div class="state" id="state">Esperando interação</div>
<p class="muted" style="text-align:center">Este é o rosto de teste no navegador. As respostas abaixo são demonstrações locais, NÃO são respostas de inteligência artificial XiaoZhi.</p>
<div class="actions">
<button id="hear" type="button">🎤 Ouvir pelo celular</button>
<button id="hello" type="button" class="secondary">🔊 Testar voz</button>
<button id="stop" type="button" class="secondary">Parar áudio</button>
</div><p id="voiceInfo" class="warn"></p></section>
<section class="card">
<h2>Primeira conversa — modo demonstração</h2>
<form id="form"><input id="message" maxlength="200" placeholder="Digite algo para o BOTIZIN" aria-label="Mensagem"/><button>Enviar</button></form>
<pre id="log" class="log" role="status">BOTIZIN: Oi! Sou o protótipo da interface de voz. Ainda não estou conectado ao servidor XiaoZhi.</pre>
</section>
<section class="card"><h2>Diagnóstico sob demanda</h2>
<div class="actions"><button id="statusBtn" class="secondary" type="button">Consultar firmware</button></div>
<pre id="firmware" class="log">Nenhuma consulta realizada.</pre>
<small>Sem polling, sem gravar flash, sem executar ações físicas.</small></section>
</main><script>
'use strict';
const $=id=>document.getElementById(id),face=$('face'),state=$('state');
let rec=null,spoken=false;
function mode(s){face.className='face '+s;state.textContent=({idle:'Esperando interação',listening:'Ouvindo',thinking:'Processando demonstração local',speaking:'Falando'}[s]||s)}
function add(who,text){$('log').textContent+='\n'+who+': '+text;$('log').scrollTop=$('log').scrollHeight}
function say(text){
  if(!('speechSynthesis' in window)){add('SISTEMA','Este navegador não suporta síntese de voz.');mode('idle');return}
  speechSynthesis.cancel();const u=new SpeechSynthesisUtterance(text);u.lang='pt-BR';u.rate=1;
  u.onstart=()=>mode('speaking');u.onend=()=>mode('idle');u.onerror=()=>mode('idle');
  spoken=true;speechSynthesis.speak(u);
}
function demo(text){
  const t=text.trim().slice(0,200);if(!t)return;
  add('VOCÊ',t);mode('thinking');
  let response='Recebi: '+t+'. Sou apenas o protótipo local do BOTIZIN. Ainda não estou conectado à inteligência artificial.';
  if(/(oi|olá|ola|bom dia|boa noite)/i.test(t))response='Olá! Eu sou o BOTIZIN! Meu rosto e minha voz no navegador já estão em teste.';
  if(/(quem é você|quem e voce|seu nome)/i.test(t))response='Sou o BOTIZIN LAB, uma interface experimental. Meu cérebro XiaoZhi ainda será integrado.';
  if(/(versão|versao|firmware)/i.test(t))response='O protótipo chama o diagnóstico da placa apenas quando você aperta Consultar firmware.';
  add('BOTIZIN',response);say(response);
}
$('form').onsubmit=e=>{e.preventDefault();const el=$('message');const t=el.value;el.value='';demo(t)};
$('hello').onclick=()=>{add('BOTIZIN','Teste do alto-falante: olá!');say('Olá! Eu sou o BOTIZIN. A saída de voz deste navegador está funcionando.')};
$('stop').onclick=()=>{if(rec)try{rec.stop()}catch(e){};if('speechSynthesis' in window)speechSynthesis.cancel();mode('idle')};
const Recognition=window.SpeechRecognition||window.webkitSpeechRecognition;
if(!window.isSecureContext){
  $('hear').disabled=true;
  $('voiceInfo').textContent='Microfone bloqueado por HTTP. Para falar pelo A22/A35, abra o painel em HTTPS seguro diretamente no celular. O acesso remoto à tela do notebook NÃO encaminha o microfone do celular.';
}else if(!Recognition){
  $('hear').disabled=true;$('voiceInfo').textContent='Este navegador não oferece reconhecimento de fala Web Speech. Use o campo de texto.';
}else{
  $('voiceInfo').textContent='Toque em Ouvir para pedir acesso ao microfone. Nada é gravado na flash.';
  $('hear').onclick=()=>{
    if(rec)try{rec.abort()}catch(e){}
    const r=new Recognition();rec=r;r.lang='pt-BR';r.interimResults=false;r.maxAlternatives=1;
    r.onstart=()=>mode('listening');
    r.onresult=e=>{const t=e.results[0][0].transcript;rec=null;demo(t)};
    r.onerror=e=>{rec=null;mode('idle');$('voiceInfo').textContent='Microfone: '+e.error};
    r.onend=()=>{if(rec===r){rec=null;mode('idle')}};
    try{r.start()}catch(e){rec=null;mode('idle');$('voiceInfo').textContent=e.message}
  };
}
$('statusBtn').onclick=async()=>{
  $('firmware').textContent='Consultando...';
  const ctl=new AbortController();const tm=setTimeout(()=>ctl.abort(),7000);
  try{const r=await fetch('/status',{cache:'no-store',signal:ctl.signal});
      if(!r.ok)throw new Error('HTTP '+r.status);
      const s=await r.text();
      $('firmware').textContent=s.split(/\r?\n/).filter(x=>/^(BOTIZIN CORE|RUNNING_PARTITION|RUNNING_OTA_STATE|LAB_MODE|WIFI|IP:|RESET_REASON)/.test(x)).join('\n')}
  catch(e){$('firmware').textContent='Falha: '+e.message}
  finally{clearTimeout(tm)}
};
</script></body></html>)BTZNLAB";
