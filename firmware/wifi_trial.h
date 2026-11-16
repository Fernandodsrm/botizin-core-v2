#pragma once
// Wi-Fi 2.0 LAB: two-phase network change. Test in RAM; explicit user confirmation
// writes NVS. A timeout/disconnect restores the previous Wi-Fi connection.
// No scan, flash writes, logging of passwords or periodic network probes here.
enum WifiTrialPhase : uint8_t {
  WIFI_TRIAL_IDLE, WIFI_TRIAL_QUEUED, WIFI_TRIAL_CONNECTING,
  WIFI_TRIAL_CONFIRM_REQUIRED, WIFI_TRIAL_COMMITTED, WIFI_TRIAL_ROLLED_BACK
};
static WifiTrialPhase wifiTrialPhase = WIFI_TRIAL_IDLE;
static String wifiTrialTargetSsid, wifiTrialTargetPassword;
static String wifiTrialOldSsid, wifiTrialOldPassword;
static const char* wifiTrialOutcome = "NONE";
static uint32_t wifiTrialDeadline = 0;
static const uint32_t WIFI_TRIAL_CONNECT_MS = 25000UL;
static const uint32_t WIFI_TRIAL_CONFIRM_MS = 120000UL;

static bool wifiTrialActive() {
  return wifiTrialPhase == WIFI_TRIAL_QUEUED ||
         wifiTrialPhase == WIFI_TRIAL_CONNECTING ||
         wifiTrialPhase == WIFI_TRIAL_CONFIRM_REQUIRED;
}
static const char* wifiTrialPhaseText() {
  switch(wifiTrialPhase) {
    case WIFI_TRIAL_QUEUED: return "QUEUED";
    case WIFI_TRIAL_CONNECTING: return "CONNECTING";
    case WIFI_TRIAL_CONFIRM_REQUIRED: return "CONFIRM_REQUIRED";
    case WIFI_TRIAL_COMMITTED: return "COMMITTED";
    case WIFI_TRIAL_ROLLED_BACK: return "ROLLED_BACK";
    default: return "IDLE";
  }
}
static uint32_t wifiTrialRemainingSeconds() {
  if(!wifiTrialActive()) return 0;
  int32_t left=(int32_t)(wifiTrialDeadline-millis());
  return left>0?((uint32_t)left+999UL)/1000UL:0;
}
static void wifiTrialForgetSecrets() {
  wifiTrialTargetPassword = "";
  wifiTrialOldPassword = "";
}
static bool wifiTrialStage(const String& requestedSsid,const String& requestedPassword) {
  if(wifiTrialActive() || !wifiManagerReady || wifiPortalOn ||
     WiFi.status()!=WL_CONNECTED ||
     !wifiOnValidCredentials(requestedSsid,requestedPassword)) return false;
  const String connected = WiFi.SSID();
  if(connected.isEmpty()) return false;
  // On older versions credentials may live in the Arduino Wi-Fi store instead
  // of botizin-net. Use the CURRENT connection's PSK when available.
  String rollbackPassword=WiFi.psk();
  if(rollbackPassword.isEmpty() && wifiManagerSavedSsid==connected)
    rollbackPassword=wifiManagerSavedPassword;
  // For unknown legacy credentials: refuse a trial rather than lose recovery.
  // A passwordless network is permitted only if explicitly stored in botizin-net.
  if(rollbackPassword.isEmpty() && wifiManagerSavedSsid!=connected) {
    wifiTrialOutcome="OLD_NETWORK_PASSWORD_UNKNOWN";
    return false;
  }
  wifiTrialOldSsid=connected;
  wifiTrialOldPassword=rollbackPassword;
  wifiTrialTargetSsid=requestedSsid;
  wifiTrialTargetSsid.trim();
  wifiTrialTargetPassword=requestedPassword;
  wifiTrialOutcome="WAITING_TO_START";
  wifiTrialPhase=WIFI_TRIAL_QUEUED;
  // Let HTTP 202 reach the browser before the old STA connection drops.
  wifiTrialDeadline=millis()+1800UL;
  return true;
}
static void wifiTrialRestore(const char* why) {
  // The saved credentials in NVS remain untouched unless explicitly confirmed.
  const String oldSsid=wifiTrialOldSsid, oldPass=wifiTrialOldPassword;
  wifiTrialPhase=WIFI_TRIAL_ROLLED_BACK;
  wifiTrialOutcome=why;
  wifiTrialDeadline=0;
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.disconnect(false,false);
  if(oldSsid.length()) WiFi.begin(oldSsid.c_str(),oldPass.c_str());
  else wifiManagerConnectKnown();
  wifiDisconnectedAt=millis();
  wifiTrialForgetSecrets();
  wifiTrialTargetSsid="";
  wifiTrialOldSsid="";
}
static bool wifiTrialConfirm() {
  if(wifiTrialPhase!=WIFI_TRIAL_CONFIRM_REQUIRED ||
     WiFi.status()!=WL_CONNECTED || WiFi.SSID()!=wifiTrialTargetSsid ||
     wifiTrialRemainingSeconds()==0) return false;
  if(!wifiManagerStoreCredentials(wifiTrialTargetSsid,wifiTrialTargetPassword)) {
    wifiTrialRestore("NVS_SAVE_FAILED");
    return false;
  }
  wifiTrialPhase=WIFI_TRIAL_COMMITTED;
  wifiTrialOutcome="CONFIRMED_AND_SAVED";
  wifiTrialDeadline=0;
  wifiTrialForgetSecrets();
  wifiTrialTargetSsid="";
  wifiTrialOldSsid="";
  WiFi.setAutoReconnect(true);
  return true;
}
static void wifiTrialTick() {
  if(!wifiTrialActive()) return;
  const uint32_t now=millis();
  if(wifiTrialPhase==WIFI_TRIAL_QUEUED) {
    if((int32_t)(now-wifiTrialDeadline)<0) return;
    wifiTrialPhase=WIFI_TRIAL_CONNECTING;
    wifiTrialOutcome="TESTING_UNSAVED";
    wifiTrialDeadline=now+WIFI_TRIAL_CONNECT_MS;
    WiFi.persistent(false);
    WiFi.setAutoReconnect(false);
    WiFi.disconnect(false,false);
    WiFi.begin(wifiTrialTargetSsid.c_str(),wifiTrialTargetPassword.c_str());
    return;
  }
  if(wifiTrialPhase==WIFI_TRIAL_CONNECTING) {
    if(WiFi.status()==WL_CONNECTED && WiFi.SSID()==wifiTrialTargetSsid) {
      wifiTrialPhase=WIFI_TRIAL_CONFIRM_REQUIRED;
      wifiTrialOutcome="CONNECTED_AWAITING_CONFIRMATION";
      wifiTrialDeadline=now+WIFI_TRIAL_CONFIRM_MS;
      return;
    }
    if((int32_t)(now-wifiTrialDeadline)>=0) wifiTrialRestore("CONNECT_TIMEOUT");
    return;
  }
  if(wifiTrialPhase==WIFI_TRIAL_CONFIRM_REQUIRED) {
    if(WiFi.status()!=WL_CONNECTED || WiFi.SSID()!=wifiTrialTargetSsid)
      wifiTrialRestore("CONNECTION_LOST");
    else if((int32_t)(now-wifiTrialDeadline)>=0)
      wifiTrialRestore("CONFIRM_TIMEOUT");
  }
}
static String wifiTrialStatus() {
  return String("WIFI_TRIAL_STATE: ")+wifiTrialPhaseText()+"\n"+
         "WIFI_TRIAL_RESULT: "+wifiTrialOutcome+"\n"+
         "WIFI_TRIAL_REMAINING_S: "+String(wifiTrialRemainingSeconds())+"\n";
}
