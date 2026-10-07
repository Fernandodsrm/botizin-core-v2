#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <NetworkClientSecure.h>
#include <HTTPClient.h>
#include <cJSON.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <esp_heap_caps.h>
#include <mbedtls/sha256.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>
#include "menu_model.h"

// Authorized temporary Menu. No downloader, upload endpoint, peer link or NVS writes.
static const char *version = "0.0.13";
static const char *moduleId = "menu_trial";
static String ssid, password, deviceToken, bootId;
static bool configured = false, oledReady = false;
static Adafruit_SSD1306 display(128, 64, &Wire, -1, 400000, 400000);
static MenuModel menu;
static const char *items[] = {"Modulos", "Conexoes", "Diagnostico", "Ajuda", "Voltar ao anterior"};
static uint32_t nextTelemetry = 0, nextRpc = 0, nextOled = 0;
static uint32_t lastLoopUs = 0, loopMaxUs = 0;
static uint32_t telemetryAttempts = 0, telemetryFailures = 0, rpcAttempts = 0;
static String telemetryStatus = "WAITING";
static uint32_t nextWifiRetry = 0;

bool hashPartition(const esp_partition_t *partition, size_t bytes, String &out) {
  if (!partition || bytes > partition->size) return false;
  mbedtls_sha256_context context;
  mbedtls_sha256_init(&context);
  bool ok = mbedtls_sha256_starts(&context, 0) == 0;
  uint8_t buffer[4096], digest[32];
  for (size_t at=0; ok && at<bytes; at+=sizeof(buffer)) {
    size_t count = min(sizeof(buffer), bytes-at);
    ok = esp_partition_read(partition, at, buffer, count) == ESP_OK &&
         mbedtls_sha256_update(&context, buffer, count) == 0;
  }
  ok = ok && mbedtls_sha256_finish(&context, digest) == 0;
  mbedtls_sha256_free(&context);
  if (!ok) return false;
  char text[65];
  for (unsigned i=0;i<32;++i) snprintf(text+i*2, 3, "%02x", digest[i]);
  text[64]=0; out=text; return true;
}
#include "timed_trial.h"

static String partitionName(const esp_partition_t *p) {
  if (!p) return "UNKNOWN";
  return String(p->label)+" / address="+String(p->address, HEX)+" / size="+String(p->size);
}
static void loadConfig() {
  Preferences prefs;
  if (!prefs.begin("wroom-config", true)) return;
  String stored = prefs.getString("config", ""); prefs.end();
  cJSON *root = cJSON_Parse(stored.c_str());
  cJSON *s=cJSON_GetObjectItemCaseSensitive(root,"ssid");
  cJSON *p=cJSON_GetObjectItemCaseSensitive(root,"password");
  cJSON *t=cJSON_GetObjectItemCaseSensitive(root,"device_token");
  if (cJSON_IsString(s) && cJSON_IsString(p) && cJSON_IsString(t)) {
    ssid=s->valuestring; password=p->valuestring; deviceToken=t->valuestring;
    configured=ssid.length()>0 && ssid.length()<=32 && deviceToken.length()>=16 && deviceToken.length()<=128;
  }
  cJSON_Delete(root);
}
static cJSON *statusObject() {
  cJSON *r=cJSON_CreateObject();
  if (!r) return nullptr;
  cJSON_AddStringToObject(r,"firmware_version",version);
  cJSON_AddStringToObject(r,"module_id",moduleId);
  cJSON_AddStringToObject(r,"boot_id",bootId.c_str());
  cJSON_AddNumberToObject(r,"uptime_seconds",millis()/1000);
  cJSON_AddNumberToObject(r,"reset_reason_code",esp_reset_reason());
  String run=partitionName(esp_ota_get_running_partition()), boot=partitionName(esp_ota_get_boot_partition());
  cJSON_AddStringToObject(r,"running_partition",run.c_str());
  cJSON_AddStringToObject(r,"boot_partition",boot.c_str());
  cJSON_AddBoolToObject(r,"timed_trial_armed",trialArmed);
  cJSON_AddStringToObject(r,"timed_trial_status",trialStatus);
  cJSON_AddBoolToObject(r,"ota_automatic_enabled",false);
  cJSON_AddBoolToObject(r,"ota_writes_blocked",true);
  cJSON_AddBoolToObject(r,"oled_initialized",oledReady);
  cJSON_AddBoolToObject(r,"buttons_configured",true);
  cJSON_AddBoolToObject(r,"wifi_connected",WiFi.status()==WL_CONNECTED);
  cJSON_AddNumberToObject(r,"ram_internal_free_bytes",heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));
  cJSON_AddNumberToObject(r,"ram_internal_min_bytes",heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));
  cJSON_AddNumberToObject(r,"ram_largest_block_bytes",heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));
  cJSON_AddNumberToObject(r,"loop_max_gap_us",loopMaxUs);
  cJSON_AddNumberToObject(r,"menu_selection",menu.selected);
  cJSON_AddNumberToObject(r,"menu_telemetry_attempts",telemetryAttempts);
  cJSON_AddNumberToObject(r,"menu_telemetry_failures",telemetryFailures);
  cJSON_AddNumberToObject(r,"menu_rpc_polls",rpcAttempts);
  return r;
}
static String cloudBase() { return "https://thingsboard.cloud/api/v1/"+deviceToken; }
static int cloudRequest(const String &suffix, const char *payload, String *body=nullptr) {
  NetworkClientSecure tls; tls.useBuiltinCACertBundle(); tls.setHandshakeTimeout(5);
  HTTPClient http; http.setConnectTimeout(5000); http.setTimeout(5000);
  http.useHTTP10(true); http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
  int code=-1000;
  if (http.begin(tls,cloudBase()+suffix)) {
    if (payload) { http.addHeader("Content-Type","application/json"); code=http.POST((uint8_t*)payload,strlen(payload)); }
    else code=http.GET();
    if (body && code==200) {
      int size=http.getSize();
      if (size>0 && size<=1024) { *body=http.getString(); if (body->length()!=(size_t)size) *body=""; }
    }
  }
  http.end(); tls.stop(); return code;
}
static void sendTelemetry() {
  cJSON *r=statusObject(); if (!r) return;
  char *payload=cJSON_PrintUnformatted(r); cJSON_Delete(r); if (!payload) return;
  ++telemetryAttempts;
  int code=cloudRequest("/telemetry",payload); cJSON_free(payload);
  telemetryStatus="HTTP_"+String(code);
  if (code<200 || code>=300) ++telemetryFailures;
  // Continue sparse retries after failures; never permanently disable telemetry.
}
static void pollRpc() {
  ++rpcAttempts;
  String body; int code=cloudRequest("/rpc?timeout=1000",nullptr,&body);
  if (code!=200 || body.isEmpty()) return;
  cJSON *root=cJSON_Parse(body.c_str());
  cJSON *id=cJSON_GetObjectItemCaseSensitive(root,"id");
  cJSON *method=cJSON_GetObjectItemCaseSensitive(root,"method");
  cJSON *params=cJSON_GetObjectItemCaseSensitive(root,"params");
  cJSON *command=cJSON_GetObjectItemCaseSensitive(params,"command_id");
  cJSON *issued=cJSON_GetObjectItemCaseSensitive(params,"issued_at_ms");
  double now=(double)time(nullptr)*1000;
  bool valid=cJSON_IsNumber(id) && id->valuedouble>=0 && id->valuedouble<=2147483647 &&
    id->valuedouble==(double)id->valueint && cJSON_IsString(method) &&
    cJSON_IsString(command) && strlen(command->valuestring)>0 && strlen(command->valuestring)<=64 &&
    cJSON_IsNumber(issued) && issued->valuedouble<=now+30000 && issued->valuedouble>=now-90000;
  if (!valid) { cJSON_Delete(root); return; }
  int requestId=id->valueint; String commandId=command->valuestring, action=method->valuestring;
  cJSON_Delete(root);
  cJSON *reply=statusObject(); if (!reply) return;
  cJSON_AddStringToObject(reply,"result",action=="ping"?"PONG":(action=="menu_status"||action=="ota_status")?"MENU_STATUS":"BLOCKED_TEMPORARY_TRIAL");
  cJSON_AddStringToObject(reply,"origin","ESP32_REAL");
  cJSON_AddStringToObject(reply,"command_id",commandId.c_str());
  cJSON_AddNumberToObject(reply,"request_id",requestId);
  char *payload=cJSON_PrintUnformatted(reply); cJSON_Delete(reply);
  if (payload) { cloudRequest("/rpc/"+String(requestId),payload); cJSON_free(payload); }
}

static void line(uint8_t row,const String &text) {
  display.setCursor(0,row<2?row*8:18+(row-2)*9); display.print(text.substring(0,21));
}
static void redraw() {
  if (!oledReady) return;
  display.clearDisplay();
  line(0,"BOTIZIN MENU - WROOM"); line(1,"Teste / volta 0.0.12");
  if (!menu.detail) {
    for (unsigned i=0;i<5;++i) line(i+2,String(i==menu.selected?"> ":"  ")+items[i]);
  } else if (menu.selected==0) {
    line(2,"LED: nao instalado"); line(3,"Instalacao bloqueada"); line(5,"Referencia preservada");
  } else if (menu.selected==1) {
    line(2,WiFi.status()==WL_CONNECTED?"Wi-Fi conectado":"Wi-Fi sem conexao");
    line(3,"IP "+WiFi.localIP().toString()); line(4,"Nuvem "+telemetryStatus);
    line(5,"Sem link com a S3");
  } else if (menu.selected==2) {
    line(2,"RAM "+String(ESP.getFreeHeap()/1024)+" KiB");
    line(3,"Loop max "+String(loopMaxUs/1000)+" ms");
    line(4,"Ligado "+String(millis()/1000)+"s");
    line(5,trialArmed?"Retorno programado":"Falha ao armar volta");
  } else if (menu.selected==3) {
    line(2,"Cima/baixo: selecionar"); line(3,"Direita: entrar / OK");
    line(4,"Esquerda: voltar"); line(5,"Retorno automatico");
  } else {
    line(2,"Voltar para 0.0.12?"); line(3,"Direita: confirmar"); line(4,"Esquerda: cancelar");
  }
  display.display();
}
static const uint8_t pins[]={25,27,32,33};
static const uint8_t flags[]={1,2,4,8};
static bool previous[4]={true,true,true,true};
static uint32_t lastPress[4]={0,0,0,0};
static void pollButtons() {
  uint8_t events=0; uint32_t now=millis();
  for (unsigned i=0;i<4;++i) {
    bool value=digitalRead(pins[i]);
    if (!value && previous[i] && (uint32_t)(now-lastPress[i])>=150) { events|=flags[i]; lastPress[i]=now; }
    previous[i]=value;
  }
  if (events && menu.handle(events) && trialArmed) esp_restart();
  if (events) nextOled=0;
}
void setup() {
  Serial.begin(115200);
  // First: verify preserved firmware, select its boot, arm independent timer.
  bool flashOK=ESP.getFlashChipSize()==4*1024*1024;
  if (flashOK) armTimedTrial(); else trialStatus="FAILED_FLASH_SIZE";
  bootId=String((unsigned long)esp_random(),HEX)+String((unsigned long)esp_random(),HEX);
  for (auto pin:pins) pinMode(pin,INPUT_PULLUP);
  if (Wire.begin(21,22,400000)) {
    Wire.setTimeOut(20); Wire.beginTransmission(0x3c);
    if (Wire.endTransmission()==0) oledReady=display.begin(SSD1306_SWITCHCAPVCC,0x3c,false,false);
    if (oledReady) { display.setTextSize(1); display.setTextColor(SSD1306_WHITE); display.setTextWrap(false); }
  }
  loadConfig(); WiFi.persistent(false); WiFi.mode(WIFI_STA); WiFi.setAutoReconnect(true);
  if (configured) WiFi.begin(ssid.c_str(),password.c_str());
  configTime(0,0,"pool.ntp.org","time.cloudflare.com");
  nextTelemetry=millis()+20000; nextRpc=millis()+30000; nextWifiRetry=millis()+30000;
  redraw(); Serial.println("BOTIZIN_MENU_TRIAL_0.0.13_WRITES_BLOCKED");
}
void loop() {
  uint32_t nowUs=micros();
  if (lastLoopUs) loopMaxUs=max(loopMaxUs,(uint32_t)(nowUs-lastLoopUs));
  lastLoopUs=nowUs; uint32_t now=millis();
  pollButtons();
  if ((int32_t)(now-nextOled)>=0) { nextOled=now+1000; redraw(); }
  if (configured && WiFi.status()!=WL_CONNECTED && (int32_t)(now-nextWifiRetry)>=0) {
    nextWifiRetry=now+30000; WiFi.reconnect();
  }
  if (configured && WiFi.status()==WL_CONNECTED && time(nullptr)>1700000000) {
    if ((int32_t)(now-nextTelemetry)>=0) { nextTelemetry=now+60000; sendTelemetry(); }
    if ((int32_t)(millis()-nextRpc)>=0) { nextRpc=millis()+15000; pollRpc(); }
  }
  delay(2);
}
