#pragma once
#include "peer_discovery_policy.h"
static PeerDiscoveryPolicy peerDiscovery;
static IPAddress peerAddress,peerNetworkIP;
static String peerAddressSource="WAITING";
// Migration fallback; never configured as this ESP32's own static IP.
static String peerManualIP="192.168.0.9";
static bool usablePeerIP(const IPAddress &ip) {
  return ip[0]!=0 && ip[0]!=127 && ip[0]<224 && ip!=WiFi.localIP();
}
static void invalidatePeerAddress() {
  peerDiscovery.invalidate();peerAddress=IPAddress();peerAddressSource="RETRY_WAIT";
}
static void loadPeerAddress() {
  Preferences p;
  if(p.begin("peer-address",true)) {peerManualIP=p.getString("ip",peerManualIP);p.end();}
}
static bool savePeerAddress(const String &text) {
  IPAddress ip;
  if(!text.isEmpty() && (!ip.fromString(text) || !usablePeerIP(ip))) return false;
  Preferences p;if(!p.begin("peer-address",false))return false;
  bool ok=p.putString("ip",text)==text.length();p.end();
  if(ok) {peerManualIP=text;invalidatePeerAddress();peerDiscovery.reset(millis());}
  return ok;
}
static String peerBaseURL() {
  if(WiFi.status()!=WL_CONNECTED) {invalidatePeerAddress();peerNetworkIP=IPAddress();return String();}
  if(peerNetworkIP!=WiFi.localIP()) {
    peerNetworkIP=WiFi.localIP();invalidatePeerAddress();peerDiscovery.reset(millis());
  }
  if(peerDiscovery.due(millis())) {
    IPAddress ip=localNameReady?MDNS.queryHost("botizin-s3",250):IPAddress();
    bool found=usablePeerIP(ip);
    peerAddressSource=found?"MDNS":"NOT_FOUND";
    if(!found && !peerManualIP.isEmpty()) {
      found=ip.fromString(peerManualIP) && usablePeerIP(ip);
      if(found)peerAddressSource="MANUAL_FALLBACK";
    }
    peerDiscovery.attempted(millis(),found);peerAddress=found?ip:IPAddress();
  }
  return peerDiscovery.cached?String("http://")+peerAddress.toString():String();
}
