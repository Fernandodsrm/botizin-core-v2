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
.ota-slots{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:12px;margin:15px 0 12px}
.ota-card{background:#102e28;border:1px solid #355849;border-radius:13px;padding:17px;min-width:0}
.ota-card.is-running{border-color:#6cdaaa;background:#133b2f}
.ota-card-head{display:flex;justify-content:space-between;align-items:center;gap:8px;margin-bottom:15px}
.ota-card-head b{font-size:.9rem}.ota-muted{font-size:.78rem;color:#a8bfb6}
.ota-tag{font-size:.73rem;font-weight:750;color:#bdd6cc;border:1px solid #517366;padding:5px 8px;border-radius:99px}
.ota-tag.is-on{background:#235d42;border-color:#71d7a0;color:#d4ffe8}
.ota-version{font-size:1.55rem;font-weight:800;letter-spacing:-.04em;overflow-wrap:anywhere;line-height:1.3}
.ota-detail{display:flex;gap:7px;flex-wrap:wrap;font-size:.78rem;color:#c0dcd0;margin-top:12px}
.ota-detail span{padding:5px 8px;background:#1b4236;border-radius:7px}
.ota-action-box{background:#102d27;border:1px solid #315046;border-radius:12px;padding:15px;margin-top:12px}
.ota-action-box h3{font-size:.94rem;margin:0 0 8px}
.ota-destination{display:block;margin:10px 0;color:#b2eacd;font-weight:700}
.ota-result{margin:12px 0 0;padding:12px;background:#173b30;border-radius:8px;font-size:.87rem}
.ota-log-details{margin-top:10px;border-top:1px solid #315046;padding-top:8px}
@media(max-width:620px){.ota-slots{grid-template-columns:1fr}.ota-version{font-size:1.4rem}}
</style></head><body>
<main>
<header><div><div class="logo">FERNANDO LAB</div><h1>BOTIZIN / Central da S3</h1></div>
<div class="flag">OTA 3.0 · CORE 0.0.30 CANDIDATA</div></header>
<p class="description">CORE 0.0.30 candidata: Wi-Fi 2.2 validado e OTA 3.0 com identificacao real dos firmwares e relatorio de instalacao.</p>
<div class="state" id="state" role="status" aria-live="polite">Nenhum diagnóstico recebido.</div>
<div class="g" id="metrics">
<div class="metric"><div class="k">Versão em execução</div><div class="v" id="firmware">—</div></div>
<div class="metric"><div class="k">Wi-Fi / HTTP</div><div class="v" id="network">—</div></div>
<div class="metric"><div class="k">Segurança de boot / OTA</div><div class="v" id="ota">—</div></div>
<div class="metric"><div class="k">RAM disponível</div><div class="v" id="memory">—</div></div>
</div>

<nav class="tabs" role="tablist" aria-label="Áreas do painel">
<button role="tab" id="tab-home" aria-controls="page-home" aria-selected="true" data-page="home">Visão geral</button>
<button role="tab" id="tab-links" aria-controls="page-links" aria-selected="false" data-page="links">Conexões</button>
<button role="tab" id="tab-wifi" aria-controls="page-wifi" aria-selected="false" data-page="wifi">Wi-Fi</button>
<button role="tab" id="tab-ota3" aria-controls="page-ota3" aria-selected="false" data-page="ota3">OTA 3.0</button>
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
<div id="page-ota3" role="tabpanel" aria-labelledby="tab-ota3" hidden>
<section class="panel">
<h2>OTA 3.0 · Gerenciar versões</h2>
<p class="muted small">Duas APPs disponíveis. Você escolhe quando instalar e quando reiniciar. O GitHub só é consultado por solicitação.</p>
<div class="ota-slots" aria-label="Firmware das duas partições">
<div class="ota-card" id="ota3-card0">
<div class="ota-card-head"><b>APP 0 <span class="ota-muted">· OTA 0</span></b><span class="ota-tag" id="ota3-tag0">CONSULTANDO</span></div>
<div class="ota-version" id="ota3-app0">Versão não consultada</div>
<div class="ota-detail" id="ota3-detail0">Aguardando consulta.</div>
</div>
<div class="ota-card" id="ota3-card1">
<div class="ota-card-head"><b>APP 1 <span class="ota-muted">· OTA 1</span></b><span class="ota-tag" id="ota3-tag1">CONSULTANDO</span></div>
<div class="ota-version" id="ota3-app1">Versão não consultada</div>
<div class="ota-detail" id="ota3-detail1">Aguardando consulta.</div>
</div></div>
<p class="tiny" id="ota3-feedback" role="status" aria-live="polite">O estado é atualizado quando esta aba é aberta ou você pede uma leitura.</p>
<div class="buttons">
<button type="button" id="ota3-status-btn">Atualizar informações das APPs</button>
<button type="button" id="ota3-catalog-btn">Consultar versões no GitHub</button>
</div>
<div class="ota-action-box">
<h3>Instalar uma versão</h3>
<label for="ota3-version" class="muted small">Escolha um firmware do catálogo</label>
<select class="control-input" id="ota3-version" disabled><option>Consulte o catálogo</option></select>
<p class="ota-destination" id="ota3-destination">Destino: aguardando identificação da APP ativa</p>
<p class="tiny">A versão da APP de destino será substituída. A instalação não altera o próximo boot.</p>
<div class="buttons"><button type="button" id="ota3-install-btn" disabled>Consultar destino para instalar</button></div>
</div>
<div class="ota-action-box">
<h3>Iniciar uma versão já instalada</h3>
<p class="muted small">Escolher uma APP reinicia a S3 nela, sem baixar ou gravar firmware.</p>
<div class="buttons">
<button type="button" id="ota3-boot0">Iniciar APP 0</button>
<button type="button" id="ota3-boot1">Iniciar APP 1</button>
</div>
</div>
<p class="ota-result" id="ota3-result" role="status" aria-live="polite">Sem operação recente nesta sessão.</p>
<details class="ota-log-details"><summary>Detalhes técnicos e progresso da operação</summary>
<pre id="ota3-output" aria-live="polite">Sem operações. Nenhuma consulta periódica.</pre>
</details>
<p class="note">Versões sem OTA 3.0 podem perder os botões de alternância. Se uma imagem não abrir o painel, a recuperação pode exigir USB.</p>
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
<div id="controlActions"><p class="note">OTA 3.0 manual: abra a aba OTA 3.0 para consultar o catálogo, instalar no slot oposto e iniciar o firmware escolhido.</p>
<div class="buttons">
<button id="controlConnections" type="button">Verificar conexões</button>
<button id="settingsGotoOta" type="button">Abrir OTA 3.0</button>
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
 const currentSlot=get(d,'RUNNING_PARTITION').includes('app0')?'APP 0':
    get(d,'RUNNING_PARTITION').includes('app1')?'APP 1':'APP desconhecida';
 $('firmware').textContent=first+' · '+currentSlot;
 $('network').textContent=wifi+' · HTTP '+(server==='YES'?'iniciado':'não confirmado');
 $('ota').textContent=state+' · '+(d.OTA_AUTOMATIC_ENABLED==='NO'?'OTA manual':'OTA automática');
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

$('settingsGotoOta').addEventListener('click',()=>$('tab-ota3').click());
const ota3Output=(message)=>{showText('ota3-output',message);};
let ota3LastSnapshot=null,ota3CatalogLoaded=false,ota3Installing=false;
function ota3VersionLabel(value) {
 if(!value||value==='NAO_IDENTIFICADA'||value==='INVALID_IMAGE'||value==='SEM_IMAGEM_RECONHECIVEL')
   return 'Versão não identificada';
 return 'CORE '+value;
}
function ota3Detail(n,data){
 const root=$('ota3-detail'+n);
 root.replaceChildren();
 const add=(value)=>{
  const span=document.createElement('span');
  span.textContent=value;
  root.append(span);
 };
 const imageValid=data['APP'+n+'_IMAGE_DESC']==='ESP_OK';
 const valid=data['APP'+n+'_STATE_DETAIL']||'ESTADO_NAO_LIDO';
 const active=data['APP'+n+'_RUNNING']==='YES';
 add(active?'Executando agora':'Instalada / inativa');
 add(data['APP'+n+'_NEXT_BOOT']==='YES'?'Próximo boot: esta APP':'Próximo boot: outra APP');
 add(valid==='VALID'?'Imagem VALID':
     valid==='SEM_REGISTRO_OTADATA'?'Ainda sem registro de boot':
     valid==='ESTADO_NAO_LIDO'?'Estado não disponível':valid.replaceAll('_',' '));
 if(!imageValid)add('Cabeçalho não validado');
 add(data['APP'+n+'_OTA3']==='YES'?'OTA 3.0':'Sem OTA 3.0');
}
function ota3UpdateDestination() {
 const d=ota3LastSnapshot;
 let running=d?.APP0_RUNNING==='YES'?0:d?.APP1_RUNNING==='YES'?1:-1;
 const target=running<0?-1:1-running;
 const busy=d?.BUSY==='YES'||ota3Installing;
 if(target<0){
  showText('ota3-destination','Destino desconhecido. Atualize as informações das APPs.');
 }else{
  const oldVersion=d['APP'+target+'_VERSION']||'desconhecida';
  showText('ota3-destination','Destino: APP '+target+' (inativa) · substitui '+
    (oldVersion==='NAO_IDENTIFICADA'?'versão não identificada':'CORE '+oldVersion));
 }
 const button=$('ota3-install-btn');
 button.textContent=target<0?'Consultar destino para instalar':'Instalar na APP '+target+' (inativa)';
 button.disabled=target<0||!ota3CatalogLoaded||busy;
}
function ota3Present(data){
 ota3LastSnapshot=data;
 for(const n of [0,1]){
  const active=data['APP'+n+'_RUNNING']==='YES';
  const el=$('ota3-card'+n);
  el.classList.toggle('is-running',active);
  const tag=$('ota3-tag'+n);
  tag.classList.toggle('is-on',active);
  tag.textContent=active?'EM EXECUÇÃO':'INSTALADA';
  showText('ota3-app'+n,ota3VersionLabel(data['APP'+n+'_VERSION']));
  ota3Detail(n,data);
 }
 ota3UpdateDestination();
 const state=data.STATE||'—',msg=data.MESSAGE||'—';
 showText('ota3-feedback','Estado da operação: '+state+' · '+msg+
   ' · lido às '+new Date().toLocaleTimeString('pt-BR'));
}
async function ota3GetStatus(){
 const reply=await runControl('/control/ota3/status');
 if(!reply)return null;
 const data=parseStatus(reply);
 if(data.APP0_RUNNING || data.APP1_RUNNING)ota3Present(data);
 return data;
}
async function ota3WatchOperation(seconds){
 const deadline=Date.now()+seconds*1000;
 while(Date.now()<deadline){
  await new Promise(resolve=>setTimeout(resolve,1200));
  const reply=await controlPost('/control/ota3/status');
  const data=parseStatus(reply);
  ota3Output(reply);
  if(data.APP0_RUNNING||data.APP1_RUNNING)ota3Present(data);
  showText('ota3-result',data.BUSY==='YES'?
      'Operação em andamento: '+(data.MESSAGE||'aguardando')+
      ' · '+(data.BYTES||'progresso indisponível')+' bytes':
      (data.STATE==='PRONTO'?'Concluído: ':'Resultado: ')+(data.MESSAGE||'—'));
  if(data.BUSY==='NO')return data;
 }
 showText('ota3-result','A consulta de progresso terminou sem confirmação final. Consulte as APPs antes de repetir.');
 ota3Output('Limite de acompanhamento no navegador. Consulte o estado manualmente.');
 return null;
}
async function ota3LoadCatalog(){
 const raw=await runControl('/control/ota3/catalog');
 if(!raw||!raw.startsWith('{'))return;
 let data;
 try{data=JSON.parse(raw)}
 catch(e){ota3Output('Catálogo inválido: '+e.message);return}
 const select=$('ota3-version'),previous=select.value;
 select.replaceChildren();
 const entries=Array.isArray(data.entries)?data.entries:[];
 for(const item of entries){
  if(!item.approved)continue;
  const option=document.createElement('option');
  option.value=item.id;
  option.textContent=item.version+' — '+item.label+
      (item.experimental?' [EXPERIMENTAL]':'')+
      (item.ota3?'':' [sem OTA3]');
  select.append(option);
 }
 if(previous && [...select.options].some(o=>o.value===previous))select.value=previous;
 ota3CatalogLoaded=select.options.length>0;
 select.disabled=!ota3CatalogLoaded;
 ota3UpdateDestination();
 showText('ota3-result',select.options.length+
   ' versões encontradas. Escolha uma para instalar na APP inativa.');
 ota3Output(select.options.length+' versões no catálogo; nenhuma instalação foi iniciada.');
}
$('tab-ota3').addEventListener('click',()=>ota3GetStatus());
$('ota3-status-btn').addEventListener('click',async()=>{
 const data=await ota3GetStatus();
 if(data)showText('ota3-result','Informações das APPs atualizadas às '+
    new Date().toLocaleTimeString('pt-BR')+'. Nenhuma instalação iniciada.');
});
$('ota3-catalog-btn').addEventListener('click',async()=>{
 const started=await runControl('/control/ota3/catalog-refresh');
 if(!started)return;
 try {
  const data=await ota3WatchOperation(55);
  if(data&&data.STATE==='PRONTO'&&data.MESSAGE==='CATALOGO_VALIDADO')
    await ota3LoadCatalog();
 }catch(e){showText('ota3-result','Consulta indisponível: '+e.message)}
});
$('ota3-version').addEventListener('change',ota3UpdateDestination);
$('ota3-install-btn').addEventListener('click',async()=>{
 const id=$('ota3-version').value;
 const data=await ota3GetStatus();
 if(!data || !id)return;
 const target=data.APP0_RUNNING==='YES'?1:data.APP1_RUNNING==='YES'?0:-1;
 if(target<0){showText('ota3-result','Destino não identificado. Instalação bloqueada.');return}
 if(!window.confirm('Instalar '+id+' na APP '+target+' (inativa)? '+
   'A versão instalada nesse slot será SUBSTITUÍDA. '+
   'A S3 continuará executando a APP '+(1-target)+' sem reiniciar.'))return;
 ota3Installing=true;ota3UpdateDestination();
 try{
  const reply=await runControl('/control/ota3/install',{id});
  if(!reply)return;
  const result=await ota3WatchOperation(240);
  if(result && result.STATE==='PRONTO' &&
       result.MESSAGE.includes('_GRAVADA_SEM_TROCAR_BOOT')){
    showText('ota3-result','Firmware gravado na APP '+target+
      '. A S3 continua na APP '+(1-target)+'. Para iniciar a nova versão, use o botão correspondente.');
  }
 }catch(e){showText('ota3-result','Falha ao acompanhar instalação: '+e.message)}
 finally{
  ota3Installing=false;
  // Refresh versions and boot state after flash, without reboot or GitHub lookup.
  await ota3GetStatus();
  ota3UpdateDestination();
 }
});
async function ota3Boot(slot){
 const data=await ota3GetStatus();if(!data)return;
 const version=data['APP'+slot+'_VERSION']||'desconhecida';
 if(!window.confirm('REINICIAR na APP '+slot+' ('+version+')? '+
    'Não haverá gravação. Se o firmware não abrir o painel, pode ser necessário USB.'))return;
 const reply=await runControl('/control/ota3/boot',{slot:String(slot)});
 if(reply){
  ota3Output(reply+'\nA conexão com o painel será interrompida.');
  showText('ota3-result','Reinício solicitado na APP '+slot+
    '. Quando a S3 voltar, a nova página mostrará a versão em execução.');
 }
}
$('ota3-boot0').addEventListener('click',()=>ota3Boot(0));
$('ota3-boot1').addEventListener('click',()=>ota3Boot(1));

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
async function refreshKnownNetworks(){
 const list=$('wf-known-list');
 const result=await wifiCommand('/control/wifi-known',{},true);
 if(!result){list.textContent='Sem resposta das redes lembradas.';return}
 let data;try{data=JSON.parse(result)}catch(e){list.textContent='Erro na lista das redes.';return}
 list.replaceChildren();
 const label=document.createElement('p');
 label.textContent=data.networks.length+' / '+data.capacity+' redes lembradas. Preferida: '+(data.preferred||'nenhuma');
 list.append(label);
 if(!data.networks.length){list.append(document.createTextNode('Nenhuma rede cadastrada.'));return}
 for(const net of data.networks){
   const row=document.createElement('div');
   row.className='control-line';row.style.margin='9px 0';
   const name=document.createElement('span');
   name.textContent=(net.active?'● ':'○ ')+net.ssid+(net.active?' — conectada':'');
   row.append(name);
   if(!net.active){
     const btn=document.createElement('button');
     btn.type='button';btn.textContent='Esquecer';btn.style.marginLeft='auto';
     btn.addEventListener('click',async()=>{
       if(!window.confirm('Esquecer a rede '+net.ssid+' e sua senha salva?'))return;
       const r=await wifiCommand('/control/wifi-forget',{ssid:net.ssid});
       if(r)refreshKnownNetworks();
     });
     row.append(btn);
   }
   list.append(row);
 }
}
$('wf-known-refresh').addEventListener('click',refreshKnownNetworks);
$('tab-wifi').addEventListener('click',()=>{loadWifiReadOnly();refreshKnownNetworks()});
$('wf-copy').addEventListener('click',async()=>{
 const el=$('wf-tested-address');el.select();el.setSelectionRange(0,el.value.length);
 try{
   if(navigator.clipboard&&window.isSecureContext)await navigator.clipboard.writeText(el.value);
   else if(!document.execCommand('copy'))throw Error('copy');
   showText('wf-feedback','Endereço copiado. Entre na rede nova antes de abrir.');
 }catch(e){showText('wf-feedback','Copie o endereço selecionado usando Ctrl+C.')}
});
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
