#pragma once
#include <ESPmDNS.h>
static bool localNameReady=false;
static IPAddress localNameIP;
static uint32_t nextLocalNameAttempt=0;
static void maintainLocalName() {
  IPAddress ip=WiFi.localIP();
  if(WiFi.status()!=WL_CONNECTED || (uint32_t)ip==0) {
    if(localNameReady) MDNS.end();
    localNameReady=false;localNameIP=IPAddress();return;
  }
  if(ip!=localNameIP) {
    if(localNameReady) MDNS.end();
    localNameReady=false;localNameIP=ip;nextLocalNameAttempt=millis();
  }
  if(!localNameReady && (int32_t)(millis()-nextLocalNameAttempt)>=0) {
    nextLocalNameAttempt=millis()+30000;
    localNameReady=MDNS.begin(BOTIZIN_LOCAL_NAME);
    if(localNameReady) {
      MDNS.addService("http","tcp",80);
      MDNS.addServiceTxt("http","tcp","board",BOTIZIN_LOCAL_BOARD);
    }
  }
}
