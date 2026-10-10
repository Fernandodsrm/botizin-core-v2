#pragma once
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "face_engine.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Existing V5 wiring. Display failures disable only this optional display.
static Adafruit_SSD1306 oled(128, 64, &Wire, -1, 400000, 400000);
static bool oledReady = false;
static SemaphoreHandle_t oledMutex=nullptr;
// One owner at a time for the framebuffer AND the entire I2C transaction.
class OledLock {
  bool held;
public:
  OledLock():held(oledMutex && xSemaphoreTake(oledMutex,pdMS_TO_TICKS(100))==pdTRUE){}
  ~OledLock(){if(held)xSemaphoreGive(oledMutex);}
  operator bool() const{return held;}
};
static uint32_t nextOledRefresh = 0;

static bool oledPresent() {
  Wire.beginTransmission(0x3c);
  return Wire.endTransmission() == 0;
}

static void oledLine(uint8_t row, const String &value) {
  // Yellow band: y=0..15. Blue content starts at y=18.
  oled.setCursor(0, row < 2 ? row * 8 : 18 + (row - 2) * 9);
  // Fixed screen width; no wrapping or oversized temporary framebuffer.
  oled.print(value.substring(0, 21));
}

static String diagnosticBytes(uint32_t n){return String(n/1024)+" KiB";}
static String diagnosticMs(uint32_t n){return String(n/1000)+" ms";}
static void oledList(const char *const *items,uint8_t count,uint8_t selected){
  uint8_t first=selected>=5?selected-4:0;
  for(uint8_t i=first;i<count&&i<first+5;++i)oledLine(2+i-first,String(i==selected?"> ":"  ")+items[i]);
}
static void diagnosticPage(){
  bool ready=diagnosticMemory.ready;
  uint32_t age=(millis()-diagnosticMemory.at)/1000;
  oledLine(0,String(navDiagnostic==0?"MEMORIA ":navDiagnostic==1?"REDE ":navDiagnostic==2?"TELA/COLETA ":"REINICIO ")+"WROOM");
  oledLine(1,ready?String("Medido ha ")+String(age)+"s":"Aguardando medidas");
  if(!ready){oledLine(2,"Coleta a cada 5s");return;}
  if(navDiagnostic==0){
    oledLine(2,"RAM livre "+diagnosticBytes(diagnosticMemory.free));
    oledLine(3,"RAM minima "+diagnosticBytes(diagnosticMemory.minimum));
    oledLine(4,"Maior bloco "+diagnosticBytes(diagnosticMemory.largest));
    oledLine(5,"PSRAM livre "+diagnosticBytes(diagnosticMemory.psram));
    oledLine(6,"Margem OTA "+diagnosticBytes(diagnosticMemory.otaFree));
  }else if(navDiagnostic==1){
    oledLine(2,"Pausa max "+diagnosticMs(diagnosticLoopMaxUs));
    oledLine(3,"Git max "+diagnosticMs(diagnosticOps[DIAG_GIT].maxUs));
    oledLine(4,"Site max "+diagnosticMs(diagnosticOps[DIAG_TB].maxUs));
    oledLine(5,"Comandos "+diagnosticMs(diagnosticOps[DIAG_RPC].maxUs));
    oledLine(6,"Max desde reinicio");
  }else if(navDiagnostic==2){
    oledLine(2,"Tela max "+diagnosticMs(diagnosticOps[DIAG_OLED].maxUs));
    oledLine(3,"Coleta max "+diagnosticMs(diagnosticOps[DIAG_SAMPLE].maxUs));
    oledLine(4,"Rosto: alvo 16 fps");
    oledLine(5,"Coleta: a cada 5s");oledLine(6,"Max desde reinicio");
  }else{
    uint32_t uptime=diagnosticUptime();
    oledLine(2,"Desde reinicio:");oledLine(3,String(uptime/60)+" min "+String(uptime%60)+"s");
    oledLine(4,"Motivo: "+resetInfo());
    oledLine(5,"ID ultimo inicio:");oledLine(6,telemetryBootId);
  }
}
static void refreshOled() {
  if(!faceDisplayEnabled.load()&&oledReady){oledReady=false;faceDisplayEnabled.store(false);oledStatus="PAUSED_I2C_ERROR";}
  if(faceActive.load()||!oledReady||uploadActive||uploadOK||rebootScheduled||(int32_t)(millis()-nextOledRefresh)<0)return;
  OledLock lock;if(!lock)return;
  nextOledRefresh=millis()+1000;DiagnosticScope diagnosticScope(DIAG_OLED);
  if(!oledPresent()){oledReady=false;faceDisplayEnabled.store(false);oledStatus="PAUSED_I2C_ERROR";diagnosticScope.failed=true;return;}
  oled.clearDisplay();
  const char *areas[]={"Conexoes","Diagnostico","Atualizacoes","Ambientes"};
  if(navLevel==0){
    oledLine(0,"BOTIZIN - WROOM");oledLine(1,"Placa independente");oledList(areas,4,oledMenuChoice);
  }else if(navLevel==2){
    oledLine(0,String("DIAGNOSTICO ")+"WROOM");oledLine(1,"Recursos e esperas");
    const char *items[]={"Memoria","Tempos da rede","Tela e coleta","Ultimo reinicio"};oledList(items,4,navDiagnostic);
  }else if(oledPage==7){diagnosticPage();
  }else if(oledPage==8){
    oledLine(0,"CONEXAO WROOM");oledLine(1,"Medido na WROOM");
      oledLine(2,WiFi.status()==WL_CONNECTED?"Wi-Fi: conectado":"Wi-Fi: desconectado");
      oledLine(3,"IP "+WiFi.localIP().toString());oledLine(4,"Versao "+String(BOTIZIN_VERSION));
      oledLine(5,telemetryStatus=="HTTP_200"?"Site: ultimo envio OK":"Site: "+telemetryStatus);
      oledLine(6,"Origem: WROOM local");
  } else if (oledPage == 2 && oledDetail && candidateReady()) {
    oledLine(0, "INSTALAR NA WROOM?");
    oledLine(1, "OK:SIM Voltar:NAO");
    oledLine(2, "Atual " + String(BOTIZIN_VERSION));
    oledLine(3, "Nova  " + otaTargetVersion);
    oledLine(4, "Vai reiniciar WROOM");
    oledLine(5, String("Prazo ") + String((otaManualUntil - millis()) / 1000) + "s");
    oledLine(6, otaAutomaticEnabled?"Automatico: LIGADO":"Automatico: DESLIGADO");
  } else if(oledPage==10) {
    oledLine(0,"AMBIENTES WROOM");oledLine(1,catalogStatus);
    uint8_t first=catalogChoice>=5?catalogChoice-4:0;
    for(uint8_t i=first;i<=environmentCount&&i<first+5;++i){
      String title=i?environments[i-1].title:String("Atualizar lista");
      if(i&&moduleAvailable&&moduleId==environments[i-1].id)title="* "+title;
      oledLine(2+i-first,String(i==catalogChoice?"> ":"  ")+title);
    }
  } else if(oledPage==11 && catalogChoice>0 && catalogChoice<=environmentCount){
    auto &e=environments[catalogChoice-1];bool installed=moduleAvailable&&moduleId==e.id;
    oledLine(0,e.title);oledLine(1,"Versao "+(installed&&otaEnvironment!=e.id?moduleVersion:e.version));
    bool downloadPrepared=oledDetail&&otaEnvironment==e.id;
    oledLine(2,downloadPrepared?"Baixar esta versao?":installed?"Instalado: abre offline":e.bytes?"Disponivel para baixar":"Atualize a lista");
    oledLine(3,installed&&!downloadPrepared?"Menu sera preservado":"Substitui outro slot");
    oledLine(4,installed?"Baixo: baixar/atual.":"Atual: "+(moduleAvailable?moduleId:String("sem ambiente")));
    oledLine(5,oledDetail?"Direita: confirmar":installed?"Direita: abrir":"Direita: preparar");
    oledLine(6,"Esquerda: voltar");
  } else if (oledPage == 2) {
    oledLine(0, String("OTA WROOM ") + BOTIZIN_VERSION);
    oledLine(1, otaAutomaticEnabled?"Automatico: LIGADO":"Automatico: DESLIGADO");
    const char *actions[]={"Consultar GitHub","Instalar nova","Cancelar pedido","Ver ambientes"};
    bool active[]={otaReady&&!internetStopped&&!otaCheckQueued&&!otaInstallQueued,candidateReady(),manualWindowActive(),true};
    for(uint8_t i=0;i<4;++i) oledLine(i+2,String(i==oledOtaChoice?"> ":"  ")+actions[i]+(active[i]?"":" [X]"));
    oledLine(6,candidateReady()?"Nova "+otaTargetVersion:internetStatus);
  }

  oled.display();
  if(!oledPresent()){oledReady=false;faceDisplayEnabled.store(false);oledStatus="PAUSED_I2C_ERROR";diagnosticScope.failed=true;}
}
static void beginOled() {
  oledMutex=xSemaphoreCreateMutex();
  if(!oledMutex){oledStatus="SKIPPED_NO_MUTEX";return;}
  // I2C timeout bounds each transaction; no reset or flash writes on failure.
  if (!Wire.begin(21, 22, 400000)) {
    oledStatus = "I2C_INIT_FAILED";
  } else {
    Wire.setTimeOut(20);
    if (!oledPresent()) oledStatus = "NOT_FOUND_0x3C";
    else if (ESP.getFreeHeap() < 82000) oledStatus = "SKIPPED_LOW_HEAP";
    else if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) oledStatus = "INIT_FAILED";
    else { oledReady = true; faceDisplayEnabled.store(true); oledStatus = "READY_128x64_SDA21_SCL22"; }
  }
  Serial.println("OLED: " + oledStatus);
  if (oledReady) {
    oled.setTextSize(1); oled.setTextColor(SSD1306_WHITE); oled.setTextWrap(false);
    refreshOled();
  }
}

// OTA runs sequentially on loopTask. Bounded OLED writes only at progress checkpoints.
void showOtaProgress(const String &phase, size_t done, size_t total) {
  pauseFace();
  if (!oledReady) return;
  OledLock lock;if(!lock)return;
  if (!oledPresent()) { oledReady = false; faceDisplayEnabled.store(false); oledStatus = "PAUSED_I2C_ERROR"; return; }
  oled.clearDisplay();
  oledLine(0, "ATUALIZANDO WROOM");
  oledLine(1, "Botoes: bloqueados");
  oledLine(2, phase);
  if (total) {
    oledLine(3, String((unsigned long)(done * 100 / total)) + "%");
    oledLine(4, String((unsigned long)done) + " bytes");
    oledLine(5, "de " + String((unsigned long)total));
  }
  oledLine(6, "Nao desligue a placa");
  oled.display();
}

// This task owns only the face and OLED. TLS, flash, navigation and Strings stay on loopTask.
static void faceTask(void *){
  BotizinFace face(esp_random());
  TickType_t lastWake=xTaskGetTickCount();
  uint32_t lastFrame=0;bool wasActive=false;
  for(;;){
    bool active=faceActive.load()&&faceDisplayEnabled.load();
    if(active){
      OledLock lock;
      if(lock && faceActive.load() && faceDisplayEnabled.load()){
        uint32_t now=millis();
        if(!oledPresent()){faceDisplayEnabled.store(false);}
        else{
          face.draw(oled,now);oled.display();
          if(!oledPresent())faceDisplayEnabled.store(false);
          else{
            faceFrames.fetch_add(1);
            if(wasActive && lastFrame){uint32_t gap=now-lastFrame;if(gap>faceMaxGapMs.load())faceMaxGapMs.store(gap);}
            lastFrame=now;
          }
        }
      }
    }
    wasActive=active;
    // Start the next deadline from now after a long pause; never burst to catch up.
    if(xTaskGetTickCount()-lastWake>pdMS_TO_TICKS(124))lastWake=xTaskGetTickCount();
    vTaskDelayUntil(&lastWake,pdMS_TO_TICKS(62));
  }
}
static void beginFaceTask(){
  faceLastActivity=millis();
  if(oledReady && xTaskCreatePinnedToCore(faceTask,"botizin-face",4096,nullptr,1,nullptr,1)!=pdPASS){
    faceActive.store(false);oledStatus="FACE_TASK_FAILED_MENU_READY";nextOledRefresh=0;
  }
}
