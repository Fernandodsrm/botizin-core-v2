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
#if BOTIZIN_LAB_OTA_DISABLED
  controlReply(403, "LAB_OTA_DISABLED");
  return;
#endif
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
#if BOTIZIN_LAB_OTA_DISABLED
  controlReply(403, "LAB_OTA_DISABLED");
  return;
#endif
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
// Wi-Fi 2.0: connected STA + admin session required for every action.
static void controlWifiTest() {
  if(!controlRequireAuth())return;
  if(rebootScheduled || uploadActive || uploadOK || peerOtaBusy || otaInstallQueued ||
     wifiTrialActive()) {controlReply(409,"WIFI_TEST_BUSY");return;}
  if(!server.hasArg("ssid") || !server.hasArg("password") ||
     !wifiOnValidCredentials(server.arg("ssid"),server.arg("password"))) {
    controlReply(400,"WIFI_CREDENTIALS_INVALID");return;
  }
  if(!wifiTrialStage(server.arg("ssid"),server.arg("password"))) {
    controlReply(409,String("WIFI_TEST_BLOCKED: ")+wifiTrialOutcome);return;
  }
  controlReply(202,"WIFI_TEST_QUEUED. Conexao antiga nao foi apagada. Aguarde e confirme em ate 120 s depois que a nova rede conectar.");
}
static void controlWifiConfirm() {
  if(!controlRequireAuth())return;
  if(wifiTrialConfirm()) controlReply(200,"WIFI_CONFIRMED_AND_SAVED");
  else controlReply(409,"WIFI_CONFIRM_UNAVAILABLE_OR_FAILED");
}
static void controlWifiTrialState() {
  if(!controlRequireAuth())return;
  controlReply(200,wifiTrialStatus());
}
static void controlWifiScan() {
  if(!controlRequireAuth())return;
  if(wifiTrialActive() || rebootScheduled || uploadActive || uploadOK || peerOtaBusy) {
    controlReply(409,"WIFI_SCAN_BUSY");return;
  }
  // Explicit request only; synchronous scan is bounded by the Wi-Fi driver.
  // Never return credentials or trigger scans from the main loop.
  const int count=WiFi.scanNetworks(false,true);
  if(count<0) {WiFi.scanDelete();controlReply(503,"WIFI_SCAN_FAILED");return;}
  cJSON *root=cJSON_CreateObject(), *list=cJSON_CreateArray();
  bool ok=root && list;
  if(ok) cJSON_AddItemToObject(root,"networks",list);
  if(ok) for(int i=0;i<count && i<20;i++) {
    String name=WiFi.SSID(i);
    if(name.isEmpty())continue;
    cJSON *item=cJSON_CreateObject();
    if(!item) {ok=false;break;}
    ok=cJSON_AddStringToObject(item,"ssid",name.c_str()) &&
       cJSON_AddNumberToObject(item,"rssi_dbm",WiFi.RSSI(i)) &&
       cJSON_AddNumberToObject(item,"channel",WiFi.channel(i)) &&
       cJSON_AddNumberToObject(item,"auth",WiFi.encryptionType(i));
    if(!ok) {cJSON_Delete(item);break;}
    cJSON_AddItemToArray(list,item);
  }
  WiFi.scanDelete();
  char *json=ok?cJSON_PrintUnformatted(root):nullptr;
  if(root)cJSON_Delete(root);
  else if(list)cJSON_Delete(list);
  if(!json) {controlReply(503,"NO_MEMORY");return;}
  server.sendHeader("Cache-Control","no-store");
  server.sendHeader("X-Content-Type-Options","nosniff");
  server.send(200,"application/json; charset=utf-8",json);
  cJSON_free(json);
}
static void controlSaveWiFi() {
  // Legacy immediate NVS/reboot path deliberately disabled in LAB.
  if(!controlRequireAuth())return;
  controlReply(409,"WIFI2_USE_TEST_AND_CONFIRM");
}
static void controlReboot() {
  if(!controlRequireAuth())return;
  if(rebootScheduled || peerOtaBusy || uploadActive || uploadOK || wifiTrialActive() ||
     otaCheckQueued || otaInstallQueued) {controlReply(409,"REBOOT_BLOCKED_BUSY");return;}
  controlReply(202,"REBOOT_SCHEDULED_IN_2_SECONDS");
  eventRecord(EV_OTA_REBOOT,EV_INFO,"PANEL_USER_REBOOT");
  rebootAt=millis()+2200UL;
  rebootScheduled=true;
}
static void controlRegisterRoutes() {
  // Header collected only for protected routes, not returned by /status or /events.
  const char *headers[]={"X-Botizin-Nonce","X-Botizin-MAC","X-Botizin-Session"};
  server.collectHeaders(headers,3);
  server.on("/control/login",HTTP_POST,controlLogin);
  server.on("/control/logout",HTTP_POST,controlLogout);
  server.on("/control/check-ota",HTTP_POST,controlCheckOTA);
  server.on("/control/ota-result",HTTP_POST,controlOTAResult);
  server.on("/control/install-ota",HTTP_POST,controlInstallOTA);
  server.on("/control/cancel-ota",HTTP_POST,controlCancelOTA);
  server.on("/control/connections",HTTP_POST,controlConnectivity);
  server.on("/control/wifi-save",HTTP_POST,controlSaveWiFi);
  server.on("/control/wifi-test",HTTP_POST,controlWifiTest);
  server.on("/control/wifi-confirm",HTTP_POST,controlWifiConfirm);
  server.on("/control/wifi-state",HTTP_POST,controlWifiTrialState);
  server.on("/control/wifi-scan",HTTP_POST,controlWifiScan);
  server.on("/control/reboot",HTTP_POST,controlReboot);
}
