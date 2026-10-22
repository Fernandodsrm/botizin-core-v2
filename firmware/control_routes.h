#pragma once
// BOTIZIN S3 local-only control UI. No background checks, polling, or remote shell.
// Uses the existing per-device SoftAP setup password, stored in device NVS.
// Local HTTP is not end-to-end encrypted: do not port-forward this endpoint.
// Sessions exist only in RAM, limited to same LAN client and ten minutes.
static String controlSession;
static IPAddress controlSessionIP;
static uint32_t controlSessionUntil=0;
static uint8_t controlBadLogins=0;
static uint32_t controlBlockedUntil=0;

static bool controlOnStaInterface() {
  return WiFi.status()==WL_CONNECTED &&
         server.client().localIP()==WiFi.localIP() &&
         server.client().localIP()!=WiFi.softAPIP();
}
static bool controlConstantEqual(const String &a,const String &b) {
  size_t n=b.length();
  uint8_t diff=(uint8_t)(a.length() ^ n);
  for(size_t i=0;i<n;i++) diff|=(uint8_t)(b[i] ^ (i<a.length()?a[i]:0));
  return diff==0;
}
static bool controlSessionValid() {
  const String provided=server.header("X-Botizin-Session");
  if(!controlOnStaInterface() || provided.length()!=32 ||
     controlSession.length()!=32 || (int32_t)(controlSessionUntil-millis())<=0 ||
     server.client().remoteIP()!=controlSessionIP) return false;
  return controlConstantEqual(provided,controlSession);
}
static bool controlRequireAuth() {
  if(controlSessionValid()) return true;
  server.sendHeader("Cache-Control","no-store");
  server.send(403,"text/plain; charset=utf-8","AUTH_REQUIRED_OR_EXPIRED");
  return false;
}
static void controlReply(int code,const String &message) {
  server.sendHeader("Cache-Control","no-store");
  server.sendHeader("X-Content-Type-Options","nosniff");
  server.send(code,"text/plain; charset=utf-8",message);
}
static void controlLogin() {
  if(!controlOnStaInterface()) {controlReply(403,"LOCAL_STA_ONLY");return;}
  if(controlBlockedUntil && (int32_t)(controlBlockedUntil-millis())>0) {
    controlReply(429,"LOGIN_TEMPORARILY_LOCKED");return;
  }
  const String pass=server.arg("password");
  if(!wifiManagerReady || pass.length()>64 || !controlConstantEqual(pass,wifiPortalPassword)) {
    if(++controlBadLogins>=5){controlBlockedUntil=millis()+90000;controlBadLogins=0;}
    controlReply(403,"INVALID_CONTROL_PASSWORD");return;
  }
  controlBadLogins=0;controlBlockedUntil=0;
  char session[33];
  for(int i=0;i<4;i++) snprintf(session+i*8,9,"%08lx",(unsigned long)esp_random());
  controlSession=String(session);
  controlSessionIP=server.client().remoteIP();
  controlSessionUntil=millis()+600000UL;
  server.sendHeader("Cache-Control","no-store");
  server.sendHeader("X-Content-Type-Options","nosniff");
  server.send(200,"text/plain; charset=utf-8",controlSession);
}
static void controlLogout() {
  if(!controlRequireAuth())return;
  controlSession="";controlSessionUntil=0;
  controlReply(200,"LOGGED_OUT");
}
static void controlCheckOTA() {
  if(!controlRequireAuth())return;
  if(!queueOtaCheck()){controlReply(409,"OTA_BUSY_OR_BLOCKED");return;}
  controlReply(202,"OTA_CHECK_QUEUED. Consult result manually.");
}
static void controlOTAResult() {
  if(!controlRequireAuth())return;
  if(otaCheckQueued || otaInstallQueued || peerOtaBusy || uploadActive || rebootScheduled){
    controlReply(202,"OTA_PROCESSING");return;
  }
  if(candidateReady()) {
    // Fixed validated fields. No secret is exposed beyond the authenticated route.
    controlReply(200,String("AVAILABLE|")+otaCandidateId+"|"+otaTargetVersion+"|"+
                       otaTargetSHA+"|"+telemetryBootId+"|"+String(otaTargetBytes));
  } else controlReply(200,String("NO_CANDIDATE|")+manualOtaStatus+"|"+internetStatus);
}
static void controlInstallOTA() {
  if(!controlRequireAuth())return;
  if(server.arg("candidate_id").length()>100 ||server.arg("target_version").length()>32 ||
     server.arg("sha256").length()!=64 ||server.arg("boot_id").length()>40) {
    controlReply(400,"INVALID_CANDIDATE");return;
  }
  if(!confirmOta(server.arg("candidate_id"),server.arg("target_version"),
                 server.arg("sha256"),server.arg("boot_id"))) {
    controlReply(409,"CANDIDATE_EXPIRED_OR_MISMATCH");return;
  }
  controlReply(202,"OTA_INSTALL_QUEUED");
}
static void controlCancelOTA() {
  if(!controlRequireAuth())return;
  if(otaInstallQueued || uploadActive || uploadOK || rebootScheduled) {
    controlReply(409,"OTA_ALREADY_IN_PROGRESS");return;
  }
  cancelOtaCheck();
  controlReply(200,"OTA_CANDIDATE_CANCELLED");
}
static void controlConnectivity() {
  if(!controlRequireAuth())return;
  // No network probes. Current state only; no additional load on external services.
  String s=String("WIFI: ")+(WiFi.status()==WL_CONNECTED?"OK":"DISCONNECTED")+"\n";
  s+="MDNS: "+String(localNameReady?"READY":"NOT_READY")+"\n";
  s+="HTTP_SERVER: "+String(eventHttpServerStarted?"STARTED":"NOT_STARTED")+"\n";
  s+="HTTP_REQUESTS: "+String(eventHttpGetCount)+"\n";
  s+="MQTT_LAST: "+botizinMqttLast+"\n";
  s+="THINGSBOARD: "+telemetryStatus+"\n";
  s+="OTA: "+internetStatus+"\n";
  controlReply(200,s);
}
static void controlSaveWiFi() {
  if(!controlRequireAuth())return;
  if(rebootScheduled || uploadActive || uploadOK || peerOtaBusy || otaInstallQueued ||
     !wifiManagerReady || !wifiOnValidCredentials(server.arg("ssid"),server.arg("password"))){
    controlReply(409,"WIFI_CHANGE_BLOCKED_OR_INVALID");return;
  }
  const String ssid=server.arg("ssid"), pass=server.arg("password");
  // Do not log either field; existing NVS namespace already used by setup portal.
  if(!wifiManagerStoreCredentials(ssid,pass)){controlReply(500,"NVS_SAVE_FAILED");return;}
  controlReply(202,"WIFI_SAVED_REBOOTING. Wrong network? Recovery AP opens after disconnect.");
  wifiRestartAfter=millis()+2600UL;
}
static void controlReboot() {
  if(!controlRequireAuth())return;
  if(rebootScheduled || peerOtaBusy || uploadActive || uploadOK ||
     otaCheckQueued || otaInstallQueued) {controlReply(409,"REBOOT_BLOCKED_BUSY");return;}
  controlReply(202,"REBOOT_SCHEDULED_IN_2_SECONDS");
  eventRecord(EV_OTA_REBOOT,EV_INFO,"PANEL_USER_REBOOT");
  rebootAt=millis()+2200UL;
  rebootScheduled=true;
}
static void controlRegisterRoutes() {
  // Header collected only for protected routes, not returned by /status or /events.
  const char *headers[]={"X-Botizin-Session"};
  server.collectHeaders(headers,1);
  server.on("/control/login",HTTP_POST,controlLogin);
  server.on("/control/logout",HTTP_POST,controlLogout);
  server.on("/control/check-ota",HTTP_POST,controlCheckOTA);
  server.on("/control/ota-result",HTTP_POST,controlOTAResult);
  server.on("/control/install-ota",HTTP_POST,controlInstallOTA);
  server.on("/control/cancel-ota",HTTP_POST,controlCancelOTA);
  server.on("/control/connections",HTTP_POST,controlConnectivity);
  server.on("/control/wifi-save",HTTP_POST,controlSaveWiFi);
  server.on("/control/reboot",HTTP_POST,controlReboot);
}
