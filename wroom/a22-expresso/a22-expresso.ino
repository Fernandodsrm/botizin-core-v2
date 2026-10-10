#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_partition.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <cJSON.h>
#include <atomic>
#include "version.h"
#include "face_engine.h"
#include "command_model.h"
#include "panel.h"
static WebServer server(80);
static Adafruit_SSD1306 oled(128,64,&Wire,-1,400000,400000);
static bool returnReady=false,oledReady=false,configured=false;
static String returnError="NOT_CHECKED",ssid,password,sessionKey;
static std::atomic<bool> returnPressed{false};
static std::atomic<uint32_t> desired{0},applied{0},frames{0},frameMaxGap{0},currentMood{0};
static uint32_t commandId=0,returnAt=0,nextReconnect=0;
static volatile uint32_t lastBack=0;
#include "menu_return.h"
#define BOTIZIN_LOCAL_NAME "botizin-wroom"
#define BOTIZIN_LOCAL_BOARD "esp32-wroom-4mb"
#include "local_name.h"
static void ARDUINO_ISR_ATTR backISR(){uint32_t now=millis();if(uint32_t(now-lastBack)>=150){lastBack=now;returnPressed.store(true);}}
static bool loadWifi(){
 Preferences p;if(!p.begin("wroom-config",true))return false;
 String stored=p.getString("config","");p.end();
 cJSON *r=cJSON_Parse(stored.c_str());if(!r)return false;
 cJSON *s=cJSON_GetObjectItemCaseSensitive(r,"ssid"),*w=cJSON_GetObjectItemCaseSensitive(r,"password");
 bool ok=cJSON_IsString(s)&&cJSON_IsString(w);
 if(ok){ssid=s->valuestring;password=w->valuestring;ok=ssid.length()>0&&ssid.length()<=32&&(password.isEmpty()||(password.length()>=8&&password.length()<=63));}
 cJSON_Delete(r);return ok;
}
static void faceTask(void*){
 BotizinFace face(esp_random());uint32_t previous=0;TickType_t tick=xTaskGetTickCount();
 for(;;){
  uint32_t now=millis(),packed=desired.load();
  if((packed>>8)!=applied.load()){face.command(uint8_t(packed),now);}
  Wire.beginTransmission(0x3c);
  if(Wire.endTransmission()==0){face.draw(oled,now);oled.display();frames.fetch_add(1);currentMood.store(face.expression());applied.store(packed>>8);}
  if(previous){uint32_t gap=now-previous;if(gap>frameMaxGap.load())frameMaxGap.store(gap);}previous=now;
  vTaskDelayUntil(&tick,pdMS_TO_TICKS(62));
 }
}
static void response(int code,const String &json){server.sendHeader("Cache-Control","no-store");server.send(code,"application/json; charset=utf-8",json);}
static bool authorized(){
 if(!returnReady){response(503,"{\"error\":\"Retorno ao menu nao validado\"}");return false;}
 if(returnAt){response(409,"{\"error\":\"Retorno em andamento\"}");return false;}
 if(server.header("X-Botizin-Key")!=sessionKey){response(403,"{\"error\":\"Recarregue a pagina para conectar\"}");return false;}
 return true;
}
static String statusJSON(){
 return String("{\"environment\":\"a22-expresso\",\"version\":\"")+A22_VERSION+"\",\"menu_return\":"+(returnReady?"true":"false")+",\"wifi\":"+(WiFi.status()==WL_CONNECTED?"true":"false")+",\"expression\":\""+expressionName(currentMood.load())+"\",\"command_id\":"+commandId+",\"applied_id\":"+applied.load()+",\"frames\":"+frames.load()+",\"frame_max_gap_ms\":"+frameMaxGap.load()+",\"free_heap\":"+ESP.getFreeHeap()+",\"uptime_seconds\":"+(millis()/1000)+"}";
}
void setup(){
 Serial.begin(115200);
 // This environment has no actuator commands. Existing servo and LED stay off.
 pinMode(14,OUTPUT);digitalWrite(14,LOW);pinMode(26,OUTPUT);digitalWrite(26,LOW);
 pinMode(32,INPUT_PULLUP);attachInterrupt(32,backISR,FALLING);
 returnReady=armMenuReturn();
 Wire.begin(21,22);Wire.setTimeOut(25);oledReady=oled.begin(SSD1306_SWITCHCAPVCC,0x3c);
 if(oledReady){
  oled.setTextSize(1);oled.setTextColor(SSD1306_WHITE);oled.clearDisplay();
  oled.setCursor(0,0);oled.print("A22 EXPRESSO ");oled.print(A22_VERSION);oled.setCursor(0,20);oled.print(returnReady?"Conectando Wi-Fi...":"Retorno nao validado");oled.display();
  if(returnReady&&xTaskCreatePinnedToCore(faceTask,"a22-eyes",4096,nullptr,1,nullptr,1)!=pdPASS){oledReady=false;}
 }
 if(!returnReady){Serial.println("MENU_RETURN: "+returnError);return;}
 configured=loadWifi();
 char key[33];snprintf(key,sizeof(key),"%08lx%08lx%08lx%08lx",(unsigned long)esp_random(),(unsigned long)esp_random(),(unsigned long)esp_random(),(unsigned long)esp_random());sessionKey=key;
 WiFi.persistent(false);WiFi.mode(WIFI_STA);WiFi.setAutoReconnect(true);
 if(configured)WiFi.begin(ssid.c_str(),password.c_str());
 const char *headers[]={"X-Botizin-Key"};server.collectHeaders(headers,1);
 server.on("/",HTTP_GET,[]{String page=FPSTR(PANEL);page.replace("__SESSION_KEY__",sessionKey);server.sendHeader("Cache-Control","no-store");server.send(200,"text/html; charset=utf-8",page);});
 server.on("/api/status",HTTP_GET,[]{response(200,statusJSON());});
 server.on("/status",HTTP_GET,[]{response(200,statusJSON());});
 server.on("/api/expression",HTTP_POST,[]{
  if(!authorized())return;
  String name=server.arg("expression");uint32_t mood;
  if(name.length()>16||!validCommand(name.c_str(),mood)){response(400,"{\"error\":\"Expressao invalida\"}");return;}
  if(!oledReady){response(503,"{\"error\":\"Tela indisponivel\"}");return;}
  commandId=commandId%0x00fffffe+1;desired.store((commandId<<8)|mood);
  response(202,String("{\"accepted\":true,\"command_id\":")+commandId+"}");
 });
 server.on("/api/return",HTTP_POST,[]{if(!authorized())return;response(202,"{\"accepted\":true}");returnAt=millis()+750;});
 server.onNotFound([]{response(404,"{\"error\":\"Rota inexistente\"}");});server.begin();nextReconnect=millis()+30000;
 Serial.println("A22 EXPRESSO V" A22_VERSION);Serial.println("MENU_RETURN: READY");
}
void loop(){
 if(returnPressed.exchange(false)&&returnReady)esp_restart();
 if(returnAt&&(int32_t)(millis()-returnAt)>=0)esp_restart();
 if(returnReady){
  server.handleClient();maintainLocalName();
  if(configured&&WiFi.status()!=WL_CONNECTED&&(int32_t)(millis()-nextReconnect)>=0){nextReconnect=millis()+30000;WiFi.reconnect();}
 }
 delay(1);
}
