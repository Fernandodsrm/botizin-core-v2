#pragma once
// On-change-only event journal. No network polling, no periodic NVS writes.
#include "event_ring.h"
static EventRing botizinEvents;
static Preferences eventPrefs;
static bool eventPrefsAvailable=false;
static uint32_t eventBootCount=0;
static bool eventHttpConfirmed=false;
static bool eventHttpServerStarted=false;
static uint32_t eventHttpGetCount=0;
static uint32_t eventHttpLastGetMs=0;
static int8_t eventWifiObserved=-1;
static int8_t eventMqttObserved=-1;
static bool eventOtaCheckReported=false;
static bool eventOtaCurrentReported=false;

// Event details MUST be fixed internal states only: no SSID/password/token/IP.
static void eventRecord(uint16_t code, EventLevel severity, const char* detail="", bool critical=false) {
  botizinEvents.add(millis(),code,severity,detail);
  Serial.printf("EVT|%lu|%s|%u|%s\n",
     (unsigned long)millis(),eventName(code),(unsigned)severity,detail?detail:"");
  // No writes for periodic telemetry/status. Only transitions marked critical.
  if(critical && eventPrefsAvailable) {
    char val[80];
    snprintf(val,sizeof(val),"%s:%s",eventName(code),detail?detail:"");
    if(eventPrefs.putString("critical",val)==0) Serial.println("EVT_NVS_CRITICAL_SAVE_FAILED");
  }
}
static void eventJournalBegin(const char* resetReason) {
  eventPrefsAvailable=eventPrefs.begin("botizin-log",false);
  String previous;
  if(eventPrefsAvailable) {
    previous=eventPrefs.getString("critical","");
    eventBootCount=eventPrefs.getUInt("boots",0)+1;
    if(!eventPrefs.putUInt("boots",eventBootCount)) Serial.println("EVT_NVS_BOOT_COUNT_SAVE_FAILED");
  }
  eventRecord(EV_BOOT,EV_INFO,resetReason?resetReason:"UNKNOWN");
  if(previous.length())eventRecord(EV_PREVIOUS_CRITICAL,EV_WARN,previous.c_str());
}
static void eventJournalObserveWiFi() {
  const int8_t current=WiFi.status()==WL_CONNECTED?1:0;
  if(eventWifiObserved==current)return;
  eventWifiObserved=current;
  eventRecord(current?EV_WIFI_CONNECTED:EV_WIFI_DISCONNECTED,current?EV_INFO:EV_WARN,
              current?"STA_CONNECTED":"STA_DISCONNECTED");
}
static void eventJournalMqttResult(bool ok) {
  const int8_t current=ok?1:0;
  if(eventMqttObserved==current)return;
  eventMqttObserved=current;
  eventRecord(ok?EV_MQTT_PUBLISHED:EV_MQTT_ERROR,ok?EV_INFO:EV_WARN,
              ok?"PUBLISH_OK":"PUBLISH_FAILED");
}
static void eventJournalHttpReceived() {
  ++eventHttpGetCount;
  eventHttpLastGetMs=millis();
  if(eventHttpConfirmed)return;
  eventHttpConfirmed=true;
  eventRecord(EV_HTTP_REQUEST_OK,EV_INFO,"FIRST_GET_SERVED");
}
static String eventJournalStatus() {
  return String("EVENTS_RAM: ")+String(botizinEvents.count)+"/64\n"+
      "EVENTS_TOTAL: "+String(botizinEvents.total)+"\n"+
      "EVENTS_OVERWRITTEN: "+String(botizinEvents.overwritten())+"\n"+
      "EVENTS_BOOT_COUNT: "+String(eventBootCount)+"\n"+
      "HTTP_SERVER_STARTED: "+String(eventHttpServerStarted?"YES":"NO")+"\n"+
      "HTTP_CONFIRMED_BY_REQUEST: "+String(eventHttpConfirmed?"YES":"NO")+"\n"+
      "HTTP_REQUESTS_SINCE_BOOT: "+String(eventHttpGetCount)+"\n"+
      "HTTP_LAST_REQUEST_AGE_S: "+String(eventHttpConfirmed? (millis()-eventHttpLastGetMs)/1000 : 0)+"\n";
}
static String eventJournalText() {
  String out;
  out.reserve(4300);
  out+="BOTIZIN EVENTS V1 (read on demand, no polling)\n";
  out+="BOOT_COUNT="+String(eventBootCount)+" TOTAL="+String(botizinEvents.total)+" OVERWRITTEN="+String(botizinEvents.overwritten())+"\n";
  for(uint8_t i=0;i<botizinEvents.count;i++) {
    const auto &e=botizinEvents.oldest(i);
    char line[105];
    snprintf(line,sizeof(line),"%lu | %s | %u | %s\n",
        (unsigned long)e.ms,eventName(e.code),(unsigned)e.level,e.detail);
    out+=line;
  }
  return out;
}
static void eventJournalRegisterRoute(WebServer& srv) {
  srv.on("/events",HTTP_GET,[](){
    eventJournalHttpReceived();
    server.sendHeader("Cache-Control","no-store");
    server.send(200,"text/plain; charset=utf-8",eventJournalText());
  });
}
