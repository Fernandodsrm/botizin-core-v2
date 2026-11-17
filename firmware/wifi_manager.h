#pragma once
// BOTIZIN S3 0.0.21: headless Wi-Fi recovery by WPA2 SoftAP and browser.
// No public/default setup password. The secret is generated ON THE DEVICE,
// stored in NVS and printed on local USB Serial (115200) when the AP starts.
// Never echo the AP password or Wi-Fi credentials over /status, MQTT or logs.
#include <DNSServer.h>

static Preferences wifiManagerPrefs;
static DNSServer wifiPortalDns;
static bool wifiPortalOn=false;
static bool wifiPortalDnsOn=false;
static bool wifiManagerReady=false;
static String wifiPortalName;
static String wifiPortalPassword;
static String wifiManagerSavedSsid;
static String wifiManagerSavedPassword;
static bool wifiOnValidCredentials(const String&,const String&);
#include "wifi_known.h"
static uint8_t wifiReconnectOrdinal=0;
static bool wifiReconnectTrying=false;
static uint32_t wifiReconnectAttemptAt=0;
static uint32_t wifiReconnectNextCycleAt=0;
static uint32_t wifiDisconnectedAt=0;
static uint32_t wifiRestartAfter=0;
static bool wifiClockConfigured=false;
// AP stays up temporarily after a successful recovery join, so the user can
// copy the assigned STA IP from http://192.168.4.1/.
static bool wifiPortalJoinQueued=false, wifiPortalJoinActive=false, wifiPortalJoinSuccess=false;
static uint32_t wifiPortalJoinAt=0, wifiPortalJoinStartedAt=0, wifiPortalCloseAt=0;
static String wifiPortalTargetSsid;
static const uint32_t WIFI_RECOVERY_MS=35000UL;

static String wifiHtmlEscape(const String &value) {
  String out;
  out.reserve(value.length()+12);
  for (size_t i=0;i<value.length();++i) {
    char c=value[i];
    if(c=='&')out+="&amp;";
    else if(c=='<')out+="&lt;";
    else if(c=='>')out+="&gt;";
    else if(c=='"')out+="&quot;";
    else if(c=='\'')out+="&#39;";
    else out+=c;
  }
  return out;
}
static bool wifiRequestFromSetupAP() {
  return wifiPortalOn && server.client().localIP()==WiFi.softAPIP();
}
static String wifiManagerStatus() {
  return String("WIFI_SETUP: ")+(wifiPortalOn?"ACTIVE":"OFF")+"\n"+
         "WIFI_SETUP_SSID: "+(wifiPortalOn?wifiPortalName:String("NOT_ACTIVE"))+"\n"+
         "WIFI_SETUP_URL: "+(wifiPortalOn?"http://192.168.4.1/":String("NOT_ACTIVE"))+"\n"+
         "WIFI_KNOWN_COUNT: "+String(wifiKnown.count)+"\n"+
         "WIFI_FALLBACK: "+String(wifiReconnectTrying?"SEARCHING":"IDLE")+"\n";
}
static String wifiPortalSuccessHTML() {
  const String ip=WiFi.localIP().toString();
  const String url=String("http://")+ip+"/";
  String body=R"PAGE(<!doctype html><html lang="pt-BR"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>BOTIZIN | Nova rede</title><style>
body{font:16px system-ui,Arial;background:#eaf2ef;color:#19342e;padding:20px}
main{max-width:530px;margin:auto;background:#fff;border:1px solid #acc7bd;border-radius:16px;padding:24px}
h1{font-size:22px}label{display:block;margin:16px 0 8px}input,button,a{font:inherit;padding:12px;border-radius:8px}
input{width:100%;box-sizing:border-box}button{cursor:pointer;background:#225e4b;color:#fff;border:none}
a{display:inline-block;background:#eef5f0;color:#1c4536}
p{line-height:1.5}</style></head><body><main>
<h1>✅ BOTIZIN conectado!</h1>
<p>O Wi-Fi novo funcionou. Este portal temporário permanece disponível por cerca de 2 minutos para você copiar o endereço.</p>
<p><b>Rede:</b> %SSID%</p>
<label for="address">Endereço da S3 na nova rede</label>
<input id="address" readonly value="%ADDRESS%">
<p><button type="button" onclick="copyIP()">Copiar endereço</button>
<a href="%ADDRESS%">Abrir painel</a></p>
<p><b>Importante:</b> conecte seu celular ou notebook à rede <b>%SSID%</b> antes de abrir o endereço acima.
O nome botizin-s3.local também pode funcionar, mas depende do dispositivo e da rede.</p>
<p><small>Se a conexão mudar de IP no futuro, o endereço antigo poderá deixar de funcionar.</small></p>
</main><script>
function copyIP(){const x=document.getElementById('address');x.select();x.setSelectionRange(0,x.value.length);
if(navigator.clipboard&&window.isSecureContext){navigator.clipboard.writeText(x.value).catch(()=>document.execCommand('copy'))}
else document.execCommand('copy');}
</script></body></html>)PAGE";
  body.replace("%SSID%",wifiHtmlEscape(WiFi.SSID()));
  body.replace("%SSID%",wifiHtmlEscape(WiFi.SSID()));
  body.replace("%ADDRESS%",wifiHtmlEscape(url));
  body.replace("%ADDRESS%",wifiHtmlEscape(url));
  return body;
}
static String wifiPortalJoiningHTML() {
  return R"PAGE(<!doctype html><html lang="pt-BR"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>BOTIZIN | Conectando</title><body style="font:16px system-ui;max-width:480px;margin:40px auto;padding:20px">
<h2>BOTIZIN está conectando à nova rede…</h2>
<p>Não precisa desligar a placa. É possível que a rede de recuperação desconecte por instantes enquanto a S3 troca o canal Wi-Fi.</p>
<p>Se a página perder conexão, reconecte à rede BOTIZIN-S3 e abra
<a href="http://192.168.4.1/">192.168.4.1</a>.</p>
<p id="result">Consultando a conexão…</p>
<script>
const x=document.getElementById('result');
async function check(){
try{
const r=await fetch('/wifi/progress',{cache:'no-store'});
if(!r.ok)throw Error('sem resposta');
const j=await r.json();
if(j.state==='CONNECTED'){window.location.replace('/');return}
if(j.state==='FAILED'){x.textContent='Não conectou. Abra novamente o portal e corrija a rede ou a senha.';return}
x.textContent='Tentando a conexão na rede selecionada…';
}catch(e){x.textContent='Acesso temporariamente interrompido. Reconecte ao BOTIZIN-S3 se necessário.'}
window.setTimeout(check,2400);
}
check();
</script></body></html>)PAGE";
}
static void wifiPortalProgress() {
  if(!wifiRequestFromSetupAP()){server.send(403,"text/plain","AP_ONLY");return;}
  const char* state=wifiPortalJoinSuccess?"CONNECTED":
                    (wifiPortalJoinActive||wifiPortalJoinQueued)?"CONNECTING":"FAILED";
  server.sendHeader("Cache-Control","no-store");
  server.send(200,"application/json; charset=utf-8",String("{\"state\":\"")+state+"\"}");
}
static void wifiPortalPage() {
  if(!wifiRequestFromSetupAP()) {
    server.send(403,"text/plain","Wi-Fi configuration is available only from setup access point.");
    return;
  }
  if(wifiPortalJoinSuccess && WiFi.status()==WL_CONNECTED) {
    server.sendHeader("Cache-Control","no-store");
    server.send(200,"text/html; charset=utf-8",wifiPortalSuccessHTML());
    return;
  }
  if(wifiPortalJoinActive || wifiPortalJoinQueued) {
    server.sendHeader("Cache-Control","no-store");
    server.send(200,"text/html; charset=utf-8",wifiPortalJoiningHTML());
    return;
  }
  String networks;
  // Scan on request, not continuously. Escape SSIDs to avoid HTML injection.
  const int count=WiFi.scanNetworks(false,true);
  if(count>0) {
    networks="<label for=\"nearby\">Redes encontradas</label>"
             "<select id=\"nearby\" onchange=\"document.getElementById('ssid').value=this.value\" "
             "style=\"width:100%;padding:12px;font-size:16px\">"
             "<option value=\"\">Escolha uma rede...</option>";
    for(int n=0;n<count && n<25;n++) {
      String ssid=wifiHtmlEscape(WiFi.SSID(n));
      if(ssid.length()==0)continue;
      networks+="<option value=\""+ssid+"\">"+ssid+"</option>";
    }
    networks+="</select>";
  }
  WiFi.scanDelete();
  String remembered;
  if(wifiKnown.count) {
    remembered="<details><summary>Redes lembradas (até 5)</summary>"
               "<p>O BOTIZIN tenta redes salvas automaticamente. "
               "Se precisar cadastrar uma sexta, esqueça uma que não usa.</p>";
    for(uint8_t i=0;i<wifiKnown.count;i++) {
      const String ssid=wifiHtmlEscape(String(wifiKnown.entries[i].ssid));
      remembered+="<form action=\"/wifi/forget\" method=\"post\" style=\"margin-bottom:8px\">"
                  "<input type=\"hidden\" name=\"ssid\" value=\""+ssid+"\">"
                  "<button type=\"submit\" style=\"margin:0;background:#6a4747\">Esquecer "+ssid+"</button></form>";
    }
    remembered+="</details>";
  }
  String body=R"HTML(<!doctype html><html lang="pt-BR"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>BOTIZIN - Configurar Wi-Fi</title><style>
body{font-family:system-ui,Arial;max-width:520px;margin:35px auto;padding:0 18px;color:#142231}
main{padding:24px;border:1px solid #ddd;border-radius:18px;background:#fafafa}
h1{font-size:24px}label{display:block;margin-top:18px;font-weight:600}
input,button{box-sizing:border-box;width:100%;padding:12px;font-size:16px;border-radius:8px}
input{border:1px solid #bbb}button{margin-top:24px;color:#fff;background:#205bb2;border:0}
p{line-height:1.45;color:#53606d}small{color:#53606d}</style></head>
<body><main><h1>BOTIZIN S3</h1>
<p>A placa nao encontrou o Wi-Fi conhecido. Digite o nome da rede 2,4 GHz
e a senha. Seus dados ficam na propria placa, nao no GitHub.</p>
<form action="/wifi/save" method="post">
<label for="ssid">Nome do Wi-Fi (SSID)</label>
<input id="ssid" name="ssid" maxlength="32" required autocomplete="off">
<label for="pass">Senha do Wi-Fi</label>
<input id="pass" name="pass" type="password" maxlength="63" autocomplete="off">
<button type="submit">Salvar e conectar</button></form>
<p><small>Depois de salvar, a S3 tenta a rede nova e mostra o IP recebido.
A rede temporária continua disponível por um curto período. Se a conexão falhar,
volte a esta página para corrigir os dados.</small></p>
</main></body></html>)HTML";
  body.replace("<form action=\"/wifi/save\" method=\"post\">",
               remembered+"<form action=\"/wifi/save\" method=\"post\">"+networks);
  server.sendHeader("Cache-Control","no-store");
  server.send(200,"text/html; charset=utf-8",body);
}
// Shared validator for captive-portal and authenticated panel Wi-Fi updates.
// Password whitespace is significant; only SSID is trimmed by caller.
static bool wifiOnValidCredentials(const String& proposedSsid,const String& proposedPassword) {
  String s=proposedSsid;s.trim();
  return !s.isEmpty() && s.length()<=32 && proposedPassword.length()<=63 &&
         (proposedPassword.length()==0 || proposedPassword.length()>=8);
}
static bool wifiManagerStoreCredentials(const String& proposedSsid,const String& pass) {
  // Explicit save: upsert in 5-network atomic list, prefer on next restart.
  // No change to the current station/STA. Full list refuses safely.
  if(!wifiManagerReady)return false;
  return wifiKnownUpsert(proposedSsid,pass,true);
}

static void wifiPortalForget() {
  if(!wifiRequestFromSetupAP()){server.send(403,"text/plain","AP_ONLY");return;}
  if(wifiPortalJoinQueued || wifiPortalJoinActive){server.send(409,"text/plain","WIFI_BUSY");return;}
  String ssid=server.arg("ssid");ssid.trim();
  const bool forgotten=ssid.length()>0 && ssid.length()<=32 && wifiKnownForget(ssid);
  server.sendHeader("Cache-Control","no-store");
  server.send(forgotten?200:409,"text/html; charset=utf-8",
     forgotten?"<!doctype html><meta charset='utf-8'><p>Rede esquecida. <a href='/'>Voltar ao portal</a></p>":
               "<!doctype html><meta charset='utf-8'><p>Não foi possível esquecer essa rede. <a href='/'>Voltar</a></p>");
}
static void wifiPortalSave() {
  if(!wifiRequestFromSetupAP()) {
    server.send(403,"text/plain","Only setup AP may change Wi-Fi.");
    return;
  }
  if(wifiPortalJoinQueued || wifiPortalJoinActive || wifiPortalJoinSuccess) {
    server.send(409,"text/plain","WIFI_PORTAL_BUSY");return;
  }
  if(!server.hasArg("ssid") || !server.hasArg("pass")) {
    server.send(400,"text/plain","Missing Wi-Fi credentials."); return;
  }
  String ssid=server.arg("ssid"), pass=server.arg("pass");
  ssid.trim();
  // WPA2 password 8..63, or empty only for an explicitly open 2.4 GHz network.
  if(!wifiOnValidCredentials(ssid,pass)){
    server.send(400,"text/plain","SSID or password length invalid.");return;
  }
  if(!wifiManagerStoreCredentials(ssid,pass)){
    server.send(500,"text/plain","NVS save failed. Retry.");return;
  }
  // Acknowledge the AP browser BEFORE switching the STA channel.
  // No forced reboot: keep the recovery AP available for the resulting IP.
  wifiPortalTargetSsid=ssid;
  wifiPortalJoinQueued=true;
  wifiPortalJoinActive=false;
  wifiPortalJoinSuccess=false;
  wifiPortalJoinAt=millis()+1800UL;
  wifiPortalCloseAt=0;
  wifiReconnectTrying=false;
  wifiReconnectNextCycleAt=0;
  server.sendHeader("Cache-Control","no-store");
  server.send(200,"text/html; charset=utf-8",wifiPortalJoiningHTML());

}
// Server-side entry gate: the dashboard HTML is served only after
// authenticating on the STA network. /status remains available for diagnostics.
static const char kBotizinLoginHTML[] PROGMEM=R"BTLOGIN(<!doctype html>
<html lang="pt-BR"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>BOTIZIN | Entrar</title><style>
:root{font-family:system-ui,Segoe UI,sans-serif;color:#e4f2ed;background:#0d1918}
*{box-sizing:border-box}body{min-height:100vh;margin:0;display:grid;place-items:center;padding:16px}
main{width:min(440px,100%);background:#132522;border:1px solid #315046;border-radius:16px;padding:28px}
h1{margin:0 0 12px;font-size:1.4rem}.small{color:#b8d0c7;font-size:.89rem;line-height:1.6}
input,button{width:100%;padding:13px;font:inherit;border-radius:9px}
input{color:#fff;background:#081512;border:1px solid #436856;margin:14px 0}
button{cursor:pointer;border:1px solid #527765;background:#1b4538;color:#fff;font-weight:700}
button:disabled{opacity:.6}#result{min-height:22px;color:#ffd59b;font-size:.9rem}
</style></head><body><main>
<h1>BOTIZIN / Central da S3</h1>
<p class="small">Uma senha para abrir o painel inteiro e usar as suas funções. Sessão local de 10 minutos de inatividade, sem guardar senha no navegador.</p>
<form id="login" autocomplete="off"><label for="password">Senha administrativa da S3</label>
<input id="password" type="password" maxlength="32" required autocomplete="off" autofocus>
<button id="enter" type="submit">Entrar no BOTIZIN</button></form>
<p id="result" role="status" aria-live="polite"></p>
<p class="small">Use apenas sua rede local confiável: este painel usa HTTP, não HTTPS.</p>
</main><script>
document.getElementById('login').addEventListener('submit',async e=>{
e.preventDefault();
const btn=document.getElementById('enter'),result=document.getElementById('result');
btn.disabled=true;result.textContent='Autenticando…';
const p=document.getElementById('password');
const password=p.value;p.value='';
const ctrl=new AbortController(),timeout=setTimeout(()=>ctrl.abort(),9000);
try{
const r=await fetch('/control/login',{method:'POST',
headers:{'Content-Type':'application/x-www-form-urlencoded'},
body:new URLSearchParams({password}),cache:'no-store',credentials:'same-origin',
redirect:'error',signal:ctrl.signal});
if(!r.ok)throw new Error('Senha ou sessão recusada (HTTP '+r.status+')');
if((await r.text()).trim()!=='AUTH_OK')throw new Error('Resposta de login inesperada');
window.location.replace('/');
}catch(err){result.textContent=err.message;btn.disabled=false}
finally{clearTimeout(timeout)}
});
</script></body></html>)BTLOGIN";
static void wifiPortalRouteHome() {
  if(wifiRequestFromSetupAP())wifiPortalPage();
  else {
    eventJournalHttpReceived();
    server.sendHeader("Cache-Control","no-store");
    server.sendHeader("X-Content-Type-Options","nosniff");
    if(!controlSessionValid()) {
      server.send_P(200,"text/html; charset=utf-8",kBotizinLoginHTML);
      return;
    }
    controlRequireAuth(); // validated above; refresh idle timeout
    server.send_P(200,"text/html; charset=utf-8",kBotizinPanelHTML);
  }
}
static void wifiManagerRegisterRoutes() {
  server.on("/wifi",HTTP_GET,wifiPortalPage);
  server.on("/wifi/save",HTTP_POST,wifiPortalSave);
  server.on("/wifi/forget",HTTP_POST,wifiPortalForget);
  server.on("/wifi/progress",HTTP_GET,wifiPortalProgress);
  // Captive-portal probes; users can always type http://192.168.4.1/.
  server.on("/generate_204",HTTP_GET,wifiPortalPage);
  server.on("/hotspot-detect.html",HTTP_GET,wifiPortalPage);
  server.on("/connecttest.txt",HTTP_GET,wifiPortalPage);
}
static void wifiPortalStart() {
  if(wifiPortalOn || !wifiManagerReady)return;
  // Keep STA active while hosting setup AP for automatic reconnection.
  WiFi.mode(WIFI_AP_STA);
  if(!WiFi.softAP(wifiPortalName.c_str(),wifiPortalPassword.c_str(),1,false,1)) {
    Serial.println("WIFI_SETUP: SOFTAP_START_FAILED");
    return;
  }
  wifiPortalOn=true;
  wifiPortalJoinSuccess=false;
  wifiPortalCloseAt=0;
  eventRecord(EV_WIFI_SETUP_ON,EV_WARN,"LOCAL_AP_RECOVERY");
  wifiPortalDnsOn=wifiPortalDns.start(53,"*",WiFi.softAPIP());
  Serial.println("WIFI_SETUP: ACTIVE");
  Serial.println("WIFI_SETUP_SSID: "+wifiPortalName);
  Serial.println("WIFI_SETUP_PASSWORD_USB_ONLY: "+wifiPortalPassword);
  Serial.println("WIFI_SETUP_URL: http://192.168.4.1/");
}
static void wifiPortalStop() {
  if(!wifiPortalOn)return;
  if(wifiPortalDnsOn){wifiPortalDns.stop();wifiPortalDnsOn=false;}
  wifiPortalOn=false;
  wifiPortalJoinQueued=false;
  wifiPortalJoinActive=false;
  wifiPortalJoinSuccess=false;
  wifiPortalCloseAt=0;
  eventRecord(EV_WIFI_SETUP_OFF,EV_INFO,"STA_RECONNECTED");
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  Serial.println("WIFI_SETUP: OFF_CONNECTED");
}
static bool wifiManagerInit() {
  if(!wifiManagerPrefs.begin("botizin-net",false)) {
    Serial.println("WIFI_SETUP: NVS_UNAVAILABLE");
    return false;
  }
  String pass=wifiManagerPrefs.getString("ap_password","");
  if(pass.length()!=16) {
    static constexpr char alphabet[]="ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz23456789";
    pass.reserve(16);
    pass="";
    for(int i=0;i<16;i++)pass+=alphabet[esp_random()%(sizeof(alphabet)-1)];
    if(wifiManagerPrefs.putString("ap_password",pass)!=16) {
      Serial.println("WIFI_SETUP: AP_SECRET_SAVE_FAILED");return false;
    }
  }
  wifiPortalPassword=pass;
  char suffix[6];
  snprintf(suffix,sizeof(suffix),"%04X",(unsigned)(ESP.getEfuseMac()&0xffffu));
  wifiPortalName=String("BOTIZIN-S3-")+suffix;
  wifiKnownLoad(); // migrates existing 0.0.27 saved Thor in RAM, no erasure
  wifiManagerSavedSsid=wifiKnownPreferred();
  wifiManagerSavedPassword=wifiKnownPasswordFor(wifiManagerSavedSsid);
  wifiManagerReady=true;
  // Show locally over USB: the device has no screen and no public password.
  Serial.println("WIFI_SETUP_SSID_USB_ONLY: "+wifiPortalName);
  Serial.println("WIFI_SETUP_PASSWORD_USB_ONLY: "+wifiPortalPassword);
  return true;
}
static bool wifiManagerConnectOrdinal(uint8_t ordinal) {
  const int ix=wifiKnownIndexByOrdinal(ordinal);
  if(ix<0)return false;
  // Keep recovery AP online if already started (AP+STA).
  WiFi.mode(wifiPortalOn?WIFI_AP_STA:WIFI_STA);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(false,false);
  WiFi.begin(wifiKnown.entries[ix].ssid,wifiKnown.entries[ix].password);
  wifiReconnectTrying=true;
  wifiReconnectOrdinal=ordinal;
  wifiReconnectAttemptAt=millis();
  return true;
}
static void wifiManagerConnectKnown() {
  wifiDisconnectedAt=millis();
  wifiReconnectTrying=false;
  wifiReconnectNextCycleAt=0;
  if(wifiKnown.count>0) {
    wifiManagerConnectOrdinal(0);
  } else {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(); // backward compatible with firmware before 0.0.21
  }
}
static void wifiManagerTick() {
  if(wifiRestartAfter && (int32_t)(millis()-wifiRestartAfter)>=0) {
    Serial.flush();ESP.restart();
  }
  // Recovery-portal setup is intentionally independent of the panel login.
  // Keep the AP for 120 s after obtaining a new IP, including on AP+STA.
  const uint32_t nowPortal=millis();
  if(wifiPortalJoinQueued && (int32_t)(nowPortal-wifiPortalJoinAt)>=0) {
    wifiPortalJoinQueued=false;wifiPortalJoinActive=true;
    wifiPortalJoinStartedAt=nowPortal;
    WiFi.mode(WIFI_AP_STA);
    WiFi.persistent(false);WiFi.setAutoReconnect(false);
    WiFi.disconnect(false,false);
    WiFi.begin(wifiPortalTargetSsid.c_str(),wifiKnownPasswordFor(wifiPortalTargetSsid).c_str());
    wifiDisconnectedAt=nowPortal;
  }
  if(wifiPortalJoinActive) {
    if(WiFi.status()==WL_CONNECTED && WiFi.SSID()==wifiPortalTargetSsid) {
      wifiPortalJoinActive=false;wifiPortalJoinSuccess=true;
      wifiPortalCloseAt=nowPortal+120000UL;
      wifiKnownSelectPreferred(WiFi.SSID());
    } else if((uint32_t)(nowPortal-wifiPortalJoinStartedAt)>=25000UL) {
      wifiPortalJoinActive=false;
      wifiPortalJoinSuccess=false;
      wifiPortalCloseAt=0;
    }
  }
  if(wifiPortalJoinQueued || wifiPortalJoinActive) {
    if(wifiPortalOn && wifiPortalDnsOn)wifiPortalDns.processNextRequest();
    return;
  }
  if(wifiPortalJoinSuccess && wifiPortalOn) {
    if(WiFi.status()!=WL_CONNECTED || WiFi.SSID()!=wifiPortalTargetSsid) {
      // Never delay failover by two minutes if the just-added Wi-Fi drops.
      wifiPortalJoinSuccess=false;
      wifiPortalCloseAt=0;
    } else {
      if(wifiPortalDnsOn)wifiPortalDns.processNextRequest();
      if((int32_t)(nowPortal-wifiPortalCloseAt)>=0)wifiPortalStop();
      else return;
    }
  }
  // Wi-Fi trial owns STA exclusively; no background credential failover here.
  if(wifiTrialActive())return;
  const uint32_t now=millis();
  if(WiFi.status()==WL_CONNECTED) {
    wifiDisconnectedAt=now;
    if(wifiReconnectTrying){
      wifiReconnectTrying=false;
      wifiReconnectNextCycleAt=0;
      wifiKnownSelectPreferred(WiFi.SSID()); // only when failover actually connected
    }
    if(wifiPortalOn && !wifiPortalJoinSuccess)wifiPortalStop();
    if(!wifiClockConfigured) {
      configTime(0,0,"pool.ntp.org","time.cloudflare.com");
      wifiClockConfigured=true;
    }
    return;
  }
  if(wifiPortalOn && wifiPortalDnsOn)wifiPortalDns.processNextRequest();
  if(!wifiDisconnectedAt)wifiDisconnectedAt=now;
  if(wifiKnown.count>0) {
    if(wifiReconnectTrying && (uint32_t)(now-wifiReconnectAttemptAt)>=11000UL) {
      if(wifiReconnectOrdinal+1<wifiKnown.count)
        wifiManagerConnectOrdinal(wifiReconnectOrdinal+1);
      else {
        wifiReconnectTrying=false;
        wifiReconnectNextCycleAt=now+45000UL;
      }
    } else if(!wifiReconnectTrying &&
              (wifiReconnectNextCycleAt==0
                ? (uint32_t)(now-wifiDisconnectedAt)>=5000UL
                : (int32_t)(now-wifiReconnectNextCycleAt)>=0)) {
      wifiReconnectNextCycleAt=0;
      wifiManagerConnectOrdinal(0);
    }
  }
  if(!wifiPortalOn && (uint32_t)(now-wifiDisconnectedAt)>=WIFI_RECOVERY_MS)
    wifiPortalStart();
}
