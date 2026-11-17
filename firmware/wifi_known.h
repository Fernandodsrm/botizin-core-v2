#pragma once
// BOTIZIN Wi-Fi 2.2: at most FIVE remembered networks in one atomic NVS blob.
// Passwords never returned by the HTTP endpoints, status, HTML or serial logs.
// The "preferred" SSID is tried first. Other credentials are fallbacks ONLY
// while STA is disconnected. Never roam merely because RSSI is better.
static const uint8_t WIFI_KNOWN_MAX=5;
struct WifiKnownEntry {
  char ssid[33];
  char password[64];
};
struct WifiKnownData {
  uint8_t schema;
  uint8_t count;
  char preferred[33];
  WifiKnownEntry entries[WIFI_KNOWN_MAX];
};
static WifiKnownData wifiKnown={};
static bool wifiKnownDirtyLegacy=false;

static int wifiKnownFind(const String& ssid) {
  for(uint8_t i=0;i<wifiKnown.count;i++)
    if(ssid==wifiKnown.entries[i].ssid)return (int)i;
  return -1;
}
static String wifiKnownPreferred() {
  return String(wifiKnown.preferred);
}
static String wifiKnownPasswordFor(const String& ssid) {
  const int index=wifiKnownFind(ssid);
  return index<0?String(""):String(wifiKnown.entries[index].password);
}
static bool wifiKnownDataValid(const WifiKnownData& d) {
  if(d.schema!=1 || d.count>WIFI_KNOWN_MAX ||
     strnlen(d.preferred,sizeof(d.preferred))==sizeof(d.preferred))return false;
  for(uint8_t i=0;i<d.count;i++) {
    if(strnlen(d.entries[i].ssid,sizeof(d.entries[i].ssid))==sizeof(d.entries[i].ssid) ||
       strnlen(d.entries[i].password,sizeof(d.entries[i].password))==sizeof(d.entries[i].password) ||
       !wifiOnValidCredentials(String(d.entries[i].ssid),String(d.entries[i].password)))return false;
    for(uint8_t j=0;j<i;j++)
      if(strcmp(d.entries[i].ssid,d.entries[j].ssid)==0)return false;
  }
  if(d.count>0) {
    bool found=false;
    for(uint8_t i=0;i<d.count;i++)
      if(strcmp(d.preferred,d.entries[i].ssid)==0)found=true;
    if(!found)return false;
  }
  return true;
}
static void wifiKnownLoad() {
  memset(&wifiKnown,0,sizeof(wifiKnown));wifiKnown.schema=1;
  WifiKnownData saved={};
  if(wifiManagerPrefs.getBytesLength("known_v1")==sizeof(saved) &&
     wifiManagerPrefs.getBytes("known_v1",&saved,sizeof(saved))==sizeof(saved) &&
     wifiKnownDataValid(saved)) {
    wifiKnown=saved;
  } else {
    // One-time in-RAM migration from the proven 0.0.27 ssid/pass fields.
    String ssid=wifiManagerPrefs.getString("ssid","");
    String pass=wifiManagerPrefs.getString("pass","");
    if(wifiOnValidCredentials(ssid,pass)) {
      ssid.toCharArray(wifiKnown.entries[0].ssid,sizeof(wifiKnown.entries[0].ssid));
      pass.toCharArray(wifiKnown.entries[0].password,sizeof(wifiKnown.entries[0].password));
      ssid.toCharArray(wifiKnown.preferred,sizeof(wifiKnown.preferred));
      wifiKnown.count=1;
    }
  }
}
static bool wifiKnownCommit(const WifiKnownData& next) {
  if(!wifiKnownDataValid(next))return false;
  // Single putBytes() preserves the previous list if NVS commit fails.
  if(wifiManagerPrefs.putBytes("known_v1",&next,sizeof(next))!=sizeof(next))return false;
  wifiKnown=next;
  const int ix=wifiKnownFind(String(next.preferred));
  wifiManagerSavedSsid=String(next.preferred);
  wifiManagerSavedPassword=ix>=0?String(next.entries[ix].password):String("");
  // Compatibility with 0.0.27 recovery; no secrets in logs.
  wifiManagerPrefs.putString("ssid",wifiManagerSavedSsid);
  wifiManagerPrefs.putString("pass",wifiManagerSavedPassword);
  return true;
}
static bool wifiKnownUpsert(const String& inputSsid,const String& password,bool prefer) {
  String ssid=inputSsid;ssid.trim();
  if(!wifiOnValidCredentials(ssid,password))return false;
  WifiKnownData next=wifiKnown;
  int index=wifiKnownFind(ssid);
  if(index<0) {
    if(next.count==WIFI_KNOWN_MAX)return false; // user must choose what to remove
    index=next.count++;
  }
  ssid.toCharArray(next.entries[index].ssid,sizeof(next.entries[index].ssid));
  password.toCharArray(next.entries[index].password,sizeof(next.entries[index].password));
  if(prefer || next.count==1)ssid.toCharArray(next.preferred,sizeof(next.preferred));
  return wifiKnownCommit(next);
}
static bool wifiKnownSelectPreferred(const String& ssid) {
  if(wifiKnownFind(ssid)<0)return false;
  if(ssid==String(wifiKnown.preferred))return true;
  WifiKnownData next=wifiKnown;
  ssid.toCharArray(next.preferred,sizeof(next.preferred));
  return wifiKnownCommit(next);
}
static bool wifiKnownForget(const String& ssid) {
  const int ix=wifiKnownFind(ssid);
  if(ix<0)return false;
  if(WiFi.status()==WL_CONNECTED && WiFi.SSID()==ssid)return false;
  WifiKnownData next=wifiKnown;
  for(uint8_t i=(uint8_t)ix;i+1<next.count;i++)next.entries[i]=next.entries[i+1];
  --next.count;
  memset(&next.entries[next.count],0,sizeof(WifiKnownEntry));
  if(ssid==String(next.preferred)) {
    memset(next.preferred,0,sizeof(next.preferred));
    if(next.count)strncpy(next.preferred,next.entries[0].ssid,sizeof(next.preferred)-1);
  }
  return wifiKnownCommit(next);
}
static int wifiKnownIndexByOrdinal(uint8_t ordinal) {
  if(wifiKnown.count==0 || ordinal>=wifiKnown.count)return -1;
  const int preferred=wifiKnownFind(String(wifiKnown.preferred));
  if(ordinal==0)return preferred>=0?preferred:0;
  uint8_t seen=1;
  for(uint8_t i=0;i<wifiKnown.count;i++){
    if((int)i==preferred)continue;
    if(seen==ordinal)return (int)i;
    ++seen;
  }
  return -1;
}
