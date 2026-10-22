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
static uint32_t wifiDisconnectedAt=0;
static uint32_t wifiRestartAfter=0;
static bool wifiClockConfigured=false;
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
         "WIFI_SETUP_URL: "+(wifiPortalOn?"http://192.168.4.1/":String("NOT_ACTIVE"))+"\n";
}
static void wifiPortalPage() {
  if(!wifiRequestFromSetupAP()) {
    server.send(403,"text/plain","Wi-Fi configuration is available only from setup access point.");
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
<p><small>Depois de salvar a S3 reinicia, tenta o Wi-Fi e desliga esta rede
temporaria. Se a senha estiver errada, o modo de configuracao reaparece.</small></p>
</main></body></html>)HTML";
  body.replace("<form action=\"/wifi/save\" method=\"post\">",
               "<form action=\"/wifi/save\" method=\"post\">"+networks);
  server.sendHeader("Cache-Control","no-store");
  server.send(200,"text/html; charset=utf-8",body);
}
static void wifiPortalSave() {
  if(!wifiRequestFromSetupAP()) {
    server.send(403,"text/plain","Only setup AP may change Wi-Fi.");
    return;
  }
  if(!server.hasArg("ssid") || !server.hasArg("pass")) {
    server.send(400,"text/plain","Missing Wi-Fi credentials."); return;
  }
  String ssid=server.arg("ssid"), pass=server.arg("pass");
  ssid.trim();
  // WPA2 password 8..63, or empty only for an explicitly open 2.4 GHz network.
  if(ssid.isEmpty() || ssid.length()>32 || pass.length()>63 ||
     (pass.length()>0 && pass.length()<8)) {
    server.send(400,"text/plain","SSID or password length invalid.");return;
  }
  if(wifiManagerPrefs.putString("ssid",ssid)!=ssid.length() ||
     wifiManagerPrefs.putString("pass",pass)!=pass.length()) {
    server.send(500,"text/plain","NVS save failed. Retry.");return;
  }
  wifiManagerSavedSsid=ssid;
  wifiManagerSavedPassword=pass;
  server.sendHeader("Cache-Control","no-store");
  server.send(200,"text/html; charset=utf-8",
      "<!doctype html><html><meta name=\"viewport\" content=\"width=device-width\"><body>"
      "<h2>Rede salva!</h2><p>BOTIZIN vai reiniciar e conectar. Se nao conseguir,"
      " esta rede de configuracao volta a aparecer.</p></body></html>");
  wifiRestartAfter=millis()+1800;
  // Deliberately avoid printing SSID/pass to serial logs.
}
static void wifiPortalRouteHome() {
  if(wifiRequestFromSetupAP())wifiPortalPage();
  else {
    eventJournalHttpReceived();
    server.sendHeader("Cache-Control","no-store");
    server.send_P(200,"text/html; charset=utf-8",kBotizinPanelHTML);
  }
}
static void wifiManagerRegisterRoutes() {
  server.on("/wifi",HTTP_GET,wifiPortalPage);
  server.on("/wifi/save",HTTP_POST,wifiPortalSave);
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
  wifiManagerSavedSsid=wifiManagerPrefs.getString("ssid","");
  wifiManagerSavedPassword=wifiManagerPrefs.getString("pass","");
  wifiManagerReady=true;
  // Show locally over USB: the device has no screen and no public password.
  Serial.println("WIFI_SETUP_SSID_USB_ONLY: "+wifiPortalName);
  Serial.println("WIFI_SETUP_PASSWORD_USB_ONLY: "+wifiPortalPassword);
  return true;
}
static void wifiManagerConnectKnown() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  if(!wifiManagerSavedSsid.isEmpty()) {
    // Keep original Arduino Wi-Fi NVS creds as fallback; our saved network
    // lives in botizin-net/ssid+pass instead.
    WiFi.persistent(false);
    WiFi.begin(wifiManagerSavedSsid.c_str(),wifiManagerSavedPassword.c_str());
  } else {
    // Backward compatible with S3 0.0.17 provisioning.
    WiFi.begin();
  }
  wifiDisconnectedAt=millis();
}
static void wifiManagerTick() {
  if(wifiRestartAfter && (int32_t)(millis()-wifiRestartAfter)>=0) {
    Serial.flush();ESP.restart();
  }
  if(WiFi.status()==WL_CONNECTED) {
    wifiDisconnectedAt=millis();
    if(wifiPortalOn)wifiPortalStop();
    if(!wifiClockConfigured) {
      configTime(0,0,"pool.ntp.org","time.cloudflare.com");
      wifiClockConfigured=true;
    }
    return;
  }
  if(wifiPortalOn && wifiPortalDnsOn)wifiPortalDns.processNextRequest();
  if(!wifiDisconnectedAt)wifiDisconnectedAt=millis();
  if(!wifiPortalOn && (uint32_t)(millis()-wifiDisconnectedAt)>=WIFI_RECOVERY_MS) {
    wifiPortalStart();
  }
}
