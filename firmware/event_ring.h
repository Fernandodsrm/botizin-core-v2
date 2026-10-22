#pragma once
// BOTIZIN event ring: fixed allocation, no heap, no networking or timers.
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

enum EventCode : uint16_t {
  EV_BOOT=1, EV_PREVIOUS_CRITICAL, EV_HW_OK, EV_HW_ERROR,
  EV_WIFI_CONNECTED, EV_WIFI_DISCONNECTED, EV_WIFI_SETUP_ON, EV_WIFI_SETUP_OFF,
  EV_HTTP_STARTED, EV_HTTP_REQUEST_OK,
  EV_OTA_CHECK, EV_OTA_CURRENT, EV_OTA_AVAILABLE, EV_OTA_START,
  EV_OTA_VERIFIED, EV_OTA_ERROR, EV_OTA_REBOOT,
  EV_MQTT_PUBLISHED, EV_MQTT_ERROR, EV_TB_ERROR, EV_TB_PAUSED
};
enum EventLevel : uint8_t { EV_INFO=1, EV_WARN=2, EV_ERROR=3 };
static inline const char* eventName(uint16_t code) {
  switch(code) {
    case EV_BOOT:return "BOOT"; case EV_PREVIOUS_CRITICAL:return "PREVIOUS_CRITICAL";
    case EV_HW_OK:return "HW_OK"; case EV_HW_ERROR:return "HW_ERROR";
    case EV_WIFI_CONNECTED:return "WIFI_CONNECTED"; case EV_WIFI_DISCONNECTED:return "WIFI_DISCONNECTED";
    case EV_WIFI_SETUP_ON:return "WIFI_SETUP_ON"; case EV_WIFI_SETUP_OFF:return "WIFI_SETUP_OFF";
    case EV_HTTP_STARTED:return "HTTP_STARTED"; case EV_HTTP_REQUEST_OK:return "HTTP_REQUEST_OK";
    case EV_OTA_CHECK:return "OTA_CHECK"; case EV_OTA_CURRENT:return "OTA_CURRENT";
    case EV_OTA_AVAILABLE:return "OTA_AVAILABLE"; case EV_OTA_START:return "OTA_START";
    case EV_OTA_VERIFIED:return "OTA_VERIFIED"; case EV_OTA_ERROR:return "OTA_ERROR";
    case EV_OTA_REBOOT:return "OTA_REBOOT";
    case EV_MQTT_PUBLISHED:return "MQTT_PUBLISHED"; case EV_MQTT_ERROR:return "MQTT_ERROR";
    case EV_TB_ERROR:return "TB_ERROR"; case EV_TB_PAUSED:return "TB_PAUSED";
    default:return "UNKNOWN";
  }
}
struct EventItem {
  uint32_t ms;
  uint16_t code;
  uint8_t level;
  char detail[48];
};
struct EventRing {
  static constexpr uint8_t CAPACITY=64;
  EventItem records[CAPACITY]{};
  uint8_t head=0;
  uint8_t count=0;
  uint32_t total=0;
  void add(uint32_t now, uint16_t code, uint8_t level, const char* detail) {
    EventItem &p=records[head];
    p.ms=now;p.code=code;p.level=level;
    snprintf(p.detail,sizeof(p.detail),"%s",detail?detail:"");
    head=(uint8_t)((head+1)%CAPACITY);
    if(count<CAPACITY)++count;
    ++total;
  }
  const EventItem& oldest(uint8_t idx) const {
    return records[(head+CAPACITY-count+idx)%CAPACITY];
  }
  uint32_t overwritten() const {return total>CAPACITY?total-CAPACITY:0;}
};
static_assert(sizeof(EventRing)<4096,"Event buffer exceeds 4KB RAM");
