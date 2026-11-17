#pragma once
// Wi-Fi 2.1: TESTE E VOLTA. Test connection in RAM, record evidence,
// automatically restore original STA and publish result after return.
// Only an explicit, separately authenticated action can store new credentials.
// No credential is logged, returned by HTTP or persisted during the test.
enum WifiTrialPhase : uint8_t {
  WIFI_TRIAL_IDLE, WIFI_TRIAL_QUEUED, WIFI_TRIAL_CONNECTING,
  WIFI_TRIAL_OBSERVING, WIFI_TRIAL_RETURNING,
  WIFI_TRIAL_TEST_OK, WIFI_TRIAL_TEST_FAILED,
  WIFI_TRIAL_RECOVERY_FAILED, WIFI_TRIAL_COMMITTED
};
static WifiTrialPhase wifiTrialPhase=WIFI_TRIAL_IDLE;
static String wifiTrialTargetSsid, wifiTrialTargetPassword;
static String wifiTrialOldSsid, wifiTrialOldPassword;
static String wifiTrialObservedIP;
static const char* wifiTrialOutcome="NONE";
static const char* wifiTrialRestoreReason="NONE";
static uint32_t wifiTrialDeadline=0, wifiTrialSaveUntil=0, wifiTrialId=0;
static bool wifiTrialConnectedToTarget=false;
static const uint32_t WIFI_TRIAL_CONNECT_MS=25000UL;
static const uint32_t WIFI_TRIAL_OBSERVE_MS=3500UL;
static const uint32_t WIFI_TRIAL_RETURN_MS=30000UL;
static const uint32_t WIFI_TRIAL_SAVE_MS=180000UL;

static bool wifiTrialActive() {
  return wifiTrialPhase==WIFI_TRIAL_QUEUED ||
         wifiTrialPhase==WIFI_TRIAL_CONNECTING ||
         wifiTrialPhase==WIFI_TRIAL_OBSERVING ||
         wifiTrialPhase==WIFI_TRIAL_RETURNING;
}
static const char* wifiTrialPhaseText() {
  switch(wifiTrialPhase) {
    case WIFI_TRIAL_QUEUED: return "QUEUED";
    case WIFI_TRIAL_CONNECTING: return "CONNECTING";
    case WIFI_TRIAL_OBSERVING: return "OBSERVING";
    case WIFI_TRIAL_RETURNING: return "RETURNING";
    case WIFI_TRIAL_TEST_OK: return "TEST_OK";
    case WIFI_TRIAL_TEST_FAILED: return "TEST_FAILED";
    case WIFI_TRIAL_RECOVERY_FAILED: return "RECOVERY_FAILED";
    case WIFI_TRIAL_COMMITTED: return "SAVED_FOR_NEXT_RESTART";
    default: return "IDLE";
  }
}
static uint32_t wifiTrialRemainingSeconds() {
  if(!wifiTrialActive()) return 0;
  int32_t left=(int32_t)(wifiTrialDeadline-millis());
  return left>0?((uint32_t)left+999UL)/1000UL:0;
}
static bool wifiTrialSaveReady() {
  return wifiTrialPhase==WIFI_TRIAL_TEST_OK &&
         (int32_t)(wifiTrialSaveUntil-millis())>0 &&
         wifiTrialTargetSsid.length()>0 &&
         WiFi.status()==WL_CONNECTED && WiFi.SSID()==wifiTrialOldSsid;
}
static void wifiTrialForgetPasswords() {
  wifiTrialTargetPassword="";
  wifiTrialOldPassword="";
}
static bool wifiTrialStage(const String& requestedSsid,const String& requestedPassword) {
  if(wifiTrialActive() || !wifiManagerReady || wifiPortalOn ||
     WiFi.status()!=WL_CONNECTED ||
     !wifiOnValidCredentials(requestedSsid,requestedPassword)) return false;
  const String connected=WiFi.SSID();
  if(connected.isEmpty()) return false;
  String rollbackPassword=wifiKnownPasswordFor(connected);
  if(rollbackPassword.isEmpty())rollbackPassword=WiFi.psk();
  if(rollbackPassword.isEmpty() && wifiManagerSavedSsid==connected)
    rollbackPassword=wifiManagerSavedPassword;
  if(rollbackPassword.isEmpty() && wifiManagerSavedSsid!=connected) {
    wifiTrialOutcome="OLD_NETWORK_PASSWORD_UNKNOWN"; return false;
  }
  wifiTrialForgetPasswords();
  wifiTrialOldSsid=connected;
  wifiTrialOldPassword=rollbackPassword;
  wifiTrialTargetSsid=requestedSsid;
  wifiTrialTargetSsid.trim();
  wifiTrialTargetPassword=requestedPassword;
  wifiTrialConnectedToTarget=false;
  wifiTrialObservedIP="";
  wifiTrialOutcome="TEST_QUEUED";
  wifiTrialRestoreReason="NONE";
  wifiTrialPhase=WIFI_TRIAL_QUEUED;
  wifiTrialDeadline=millis()+1800UL;
  wifiTrialSaveUntil=0;
  ++wifiTrialId;
  return true;
}
static void wifiTrialBeginReturn(const char* reason) {
  const String oldSsid=wifiTrialOldSsid, oldPass=wifiTrialOldPassword;
  wifiTrialRestoreReason=reason;
  wifiTrialOutcome=wifiTrialConnectedToTarget?"TARGET_CONNECTED_RETURNING":"TARGET_FAILED_RETURNING";
  wifiTrialPhase=WIFI_TRIAL_RETURNING;
  wifiTrialDeadline=millis()+WIFI_TRIAL_RETURN_MS;
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.disconnect(false,false);
  if(oldSsid.length()) WiFi.begin(oldSsid.c_str(),oldPass.c_str());
  else wifiManagerConnectKnown();
  wifiDisconnectedAt=millis();
  // Original password is no longer needed: WiFi.begin has copied it.
  wifiTrialOldPassword="";
  if(!wifiTrialConnectedToTarget) wifiTrialTargetPassword="";
}
static bool wifiTrialSave() {
  // Save only for NEXT restart. Stay on original Wi-Fi so browser stays reachable.
  if(!wifiTrialSaveReady()) return false;
  if(!wifiManagerStoreCredentials(wifiTrialTargetSsid,wifiTrialTargetPassword)) {
    wifiTrialOutcome=wifiKnown.count==WIFI_KNOWN_MAX && wifiKnownFind(wifiTrialTargetSsid)<0 ?
      "MAX_FIVE_NETWORKS_FORGET_ONE":"NVS_SAVE_FAILED";return false;
  }
  wifiTrialPhase=WIFI_TRIAL_COMMITTED;
  wifiTrialOutcome="SAVED_FOR_NEXT_RESTART";
  wifiTrialSaveUntil=0;
  wifiTrialForgetPasswords();
  return true;
}
static void wifiTrialTick() {
  const uint32_t now=millis();
  if(wifiTrialPhase==WIFI_TRIAL_TEST_OK &&
     (int32_t)(now-wifiTrialSaveUntil)>=0) {
    wifiTrialTargetPassword="";
    wifiTrialSaveUntil=0;
  }
  if(!wifiTrialActive()) return;
  if(wifiTrialPhase==WIFI_TRIAL_QUEUED) {
    if((int32_t)(now-wifiTrialDeadline)<0) return;
    wifiTrialPhase=WIFI_TRIAL_CONNECTING;
    wifiTrialOutcome="CONNECTING_TO_TARGET";
    wifiTrialDeadline=now+WIFI_TRIAL_CONNECT_MS;
    WiFi.persistent(false);
    WiFi.setAutoReconnect(false);
    WiFi.disconnect(false,false);
    WiFi.begin(wifiTrialTargetSsid.c_str(),wifiTrialTargetPassword.c_str());
    return;
  }
  if(wifiTrialPhase==WIFI_TRIAL_CONNECTING) {
    if(WiFi.status()==WL_CONNECTED && WiFi.SSID()==wifiTrialTargetSsid) {
      wifiTrialConnectedToTarget=true;
      wifiTrialObservedIP=WiFi.localIP().toString();
      wifiTrialPhase=WIFI_TRIAL_OBSERVING;
      wifiTrialOutcome="TARGET_CONNECTED_CHECKING_STABILITY";
      wifiTrialDeadline=now+WIFI_TRIAL_OBSERVE_MS;
      return;
    }
    if((int32_t)(now-wifiTrialDeadline)>=0) wifiTrialBeginReturn("CONNECT_TIMEOUT");
    return;
  }
  if(wifiTrialPhase==WIFI_TRIAL_OBSERVING) {
    if(WiFi.status()!=WL_CONNECTED || WiFi.SSID()!=wifiTrialTargetSsid) {
      wifiTrialConnectedToTarget=false;
      wifiTrialBeginReturn("TARGET_CONNECTION_LOST");
    } else if((int32_t)(now-wifiTrialDeadline)>=0) {
      wifiTrialBeginReturn("TARGET_CONNECTED");
    }
    return;
  }
  if(wifiTrialPhase==WIFI_TRIAL_RETURNING) {
    if(WiFi.status()==WL_CONNECTED && WiFi.SSID()==wifiTrialOldSsid) {
      wifiTrialPhase=wifiTrialConnectedToTarget?WIFI_TRIAL_TEST_OK:WIFI_TRIAL_TEST_FAILED;
      wifiTrialOutcome=wifiTrialConnectedToTarget?
        "CONNECTED_TO_TARGET_AND_RETURNED":"TEST_FAILED_RETURNED_TO_ORIGINAL";
      wifiTrialDeadline=0;
      wifiTrialSaveUntil=wifiTrialConnectedToTarget?now+WIFI_TRIAL_SAVE_MS:0;
      wifiTrialOldPassword="";
      if(!wifiTrialConnectedToTarget) wifiTrialTargetPassword="";
      return;
    }
    if((int32_t)(now-wifiTrialDeadline)>=0) {
      wifiTrialPhase=WIFI_TRIAL_RECOVERY_FAILED;
      wifiTrialOutcome="ORIGINAL_WIFI_NOT_RECONNECTED";
      wifiTrialForgetPasswords();
      wifiTrialSaveUntil=0;
      wifiDisconnectedAt=now-WIFI_RECOVERY_MS;
      // The existing recovery AP may now start in wifiManagerTick().
    }
  }
}
static String wifiTrialStatus() {
  return String("WIFI_TRIAL_STATE: ")+wifiTrialPhaseText()+"\n"+
         "WIFI_TRIAL_RESULT: "+wifiTrialOutcome+"\n"+
         "WIFI_TRIAL_REASON: "+wifiTrialRestoreReason+"\n"+
         "WIFI_TRIAL_ID: "+String(wifiTrialId)+"\n"+
         "WIFI_TRIAL_TARGET: "+wifiTrialTargetSsid+"\n"+
         "WIFI_TRIAL_TARGET_IP: "+wifiTrialObservedIP+"\n"+
         "WIFI_TRIAL_RETURNED_WIFI: "+String(WiFi.status()==WL_CONNECTED ? WiFi.SSID() : "DISCONNECTED")+"\n"+
         "WIFI_TRIAL_CAN_SAVE: "+String(wifiTrialSaveReady()?"YES":"NO")+"\n"+
         "WIFI_TRIAL_REMAINING_S: "+String(wifiTrialRemainingSeconds())+"\n";
}
