#pragma once
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Existing V5 wiring. Display failures disable only this optional display.
static Adafruit_SSD1306 oled(128, 64, &Wire, -1, 400000, 400000);
static bool oledReady = false;
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
  bool remote=navBoard;bool ready=remote?peerHaveReport&&peerFrame.diagnostics:diagnosticMemory.ready;
  uint32_t age=remote?(millis()-peerSeenAt)/1000+peerFrame.sampleAge:(millis()-diagnosticMemory.at)/1000;
  oledLine(0,String(navDiagnostic==0?"MEMORIA ":navDiagnostic==1?"REDE ":navDiagnostic==2?"TELA/COLETA ":"REINICIO ")+(remote?"S3":"WROOM"));
  oledLine(1,ready?(remote&&!peerRecent()?"Dados antigos ":"Medido ha ")+String(age)+"s":"Aguardando medidas");
  if(!ready){oledLine(2,remote?"S3 precisa 0.0.12+":"Coleta a cada 5s");return;}
  if(navDiagnostic==0){
    oledLine(2,"RAM livre "+diagnosticBytes(remote?peerFrame.free:diagnosticMemory.free));
    oledLine(3,"RAM minima "+diagnosticBytes(remote?peerFrame.minimum:diagnosticMemory.minimum));
    oledLine(4,"Maior bloco "+diagnosticBytes(remote?peerFrame.largest:diagnosticMemory.largest));
    oledLine(5,"PSRAM livre "+diagnosticBytes(remote?peerFrame.psram:diagnosticMemory.psram));
    oledLine(6,"Margem OTA "+diagnosticBytes(remote?peerFrame.otaFree:diagnosticMemory.otaFree));
  }else if(navDiagnostic==1){
    oledLine(2,"Pausa max "+diagnosticMs(remote?peerFrame.loopMax:diagnosticLoopMaxUs));
    oledLine(3,"Git max "+diagnosticMs(remote?peerFrame.gitMax:diagnosticOps[DIAG_GIT].maxUs));
    oledLine(4,"Site max "+diagnosticMs(remote?peerFrame.tbMax:diagnosticOps[DIAG_TB].maxUs));
    oledLine(5,"Comandos "+diagnosticMs(remote?peerFrame.rpcMax:diagnosticOps[DIAG_RPC].maxUs));
    oledLine(6,"Link max "+diagnosticMs(remote?peerFrame.peerMax:diagnosticOps[DIAG_PEER].maxUs));
  }else if(navDiagnostic==2){
    oledLine(2,"Tela max "+diagnosticMs(remote?peerFrame.oledMax:diagnosticOps[DIAG_OLED].maxUs));
    oledLine(3,"Coleta max "+diagnosticMs(remote?peerFrame.sampleMax:diagnosticOps[DIAG_SAMPLE].maxUs));
    oledLine(4,remote?"S3 sem OLED local":"Tela: ate 1 vez/s");
    oledLine(5,"Coleta: a cada 5s");oledLine(6,"Max desde reinicio");
  }else{
    uint32_t uptime=remote?peerFrame.uptime:diagnosticUptime();
    oledLine(2,"Desde reinicio:");oledLine(3,String(uptime/60)+" min "+String(uptime%60)+"s");
    oledLine(4,"Motivo: "+(remote?String(peerFrame.reset):resetInfo()));
    oledLine(5,"ID ultimo inicio:");oledLine(6,remote?String(peerFrame.bootId):telemetryBootId);
  }
}
static void refreshOled() {
  if(!oledReady||uploadActive||uploadOK||rebootScheduled||(int32_t)(millis()-nextOledRefresh)<0)return;
  nextOledRefresh=millis()+1000;DiagnosticScope diagnosticScope(DIAG_OLED);
  if(!oledPresent()){oledReady=false;oledStatus="PAUSED_I2C_ERROR";diagnosticScope.failed=true;return;}
  oled.clearDisplay();
  const char *areas[]={"Conexoes","Diagnostico","Atualizacoes","Controles","Ajuda"};
  if(navLevel==0){
    oledLine(0,"BOTIZIN - CONJUNTO");oledLine(1,"WROOM + S3");oledList(areas,5,oledMenuChoice);
  }else if(navLevel==1){
    oledLine(0,areas[navGroup]);oledLine(1,"Selecionar placa");
    const char *boards[]={"WROOM - robo","S3 - central"};oledList(boards,2,navBoard);
  }else if(navLevel==2){
    oledLine(0,String("DIAGNOSTICO ")+(navBoard?"S3":"WROOM"));oledLine(1,"Recursos e esperas");
    const char *items[]={"Memoria","Tempos da rede","Tela e coleta","Ultimo reinicio"};oledList(items,4,navDiagnostic);
  }else if(oledPage==7){diagnosticPage();
  }else if(oledPage==8){
    bool remote=navBoard;oledLine(0,String("CONEXAO ")+(remote?"S3":"WROOM"));
    oledLine(1,remote?"Recebido da S3":"Medido na WROOM");
    if(remote){
      oledLine(2,peerRecent()?"Wi-Fi: conectado":"Wi-Fi: sem dado atual");
      oledLine(3,peerHaveReport?"IP "+String(peerFrame.ip):"IP: aguardando");
      oledLine(4,peerHaveReport?"Versao "+String(peerFrame.version):"Versao: aguardando");
      oledLine(5,peerHaveReport?"Resposta ha "+String((millis()-peerSeenAt)/1000)+"s":"Sem resposta recebida");
      oledLine(6,peerRecent()?"Link entre placas: OK":"Link: sem dado atual");
    }else{
      oledLine(2,WiFi.status()==WL_CONNECTED?"Wi-Fi: conectado":"Wi-Fi: desconectado");
      oledLine(3,"IP "+WiFi.localIP().toString());oledLine(4,"Versao "+String(BOTIZIN_VERSION));
      oledLine(5,telemetryStatus=="HTTP_200"?"Site: ultimo envio OK":"Site: "+telemetryStatus);
      oledLine(6,"Origem: WROOM local");
    }
  }else if(oledPage==9){
    oledLine(0,String("CONTROLES ")+(navBoard?"S3":"WROOM"));oledLine(1,"Funcoes disponiveis");
    oledLine(2,"  LED [X]");oledLine(4,"Ainda nao implantado");
  } else if (oledPage == 2 && oledDetail && candidateReady()) {
    oledLine(0, "INSTALAR NA WROOM?");
    oledLine(1, "OK:SIM Voltar:NAO");
    oledLine(2, "Atual " + String(BOTIZIN_VERSION));
    oledLine(3, "Nova  " + otaTargetVersion);
    oledLine(4, "Vai reiniciar WROOM");
    oledLine(5, String("Prazo ") + String((otaManualUntil - millis()) / 1000) + "s");
    oledLine(6, "Automatico: LIGADO");
  } else if (oledPage == 2) {
    oledLine(0, String("OTA WROOM ") + BOTIZIN_VERSION + "");
    oledLine(1, "Automatico: LIGADO");
    const char *actions[] = {"Consultar GitHub", "Instalar nova", "Cancelar pedido"};
    bool active[] = {otaReady && !internetStopped, candidateReady(), manualWindowActive()};
    for (uint8_t i = 0; i < 3; ++i) {
      oledLine(i + 2, String(i == oledOtaChoice ? "> " : "  ") + actions[i] + (active[i] ? "" : " [X]"));
    }
    oledLine(5, candidateReady() ? "Nova " + otaTargetVersion + " Auto:ON" : internetStatus.startsWith("UP_TO_DATE") ? "Auto:ON | sem nova" : "Automatico: LIGADO");
    oledLine(6, active[oledOtaChoice] ? (oledOtaChoice == 0 ? "Busca; nao instala" : oledOtaChoice == 1 ? "OK: abre confirmacao" : "Auto retoma em 5 min") : oledOtaChoice == 1 ? "Sem candidata valida" : oledOtaChoice == 2 ? "Sem pedido manual" : "Consulta bloqueada");
  } else if (oledPage == 5 && oledDetail && s3CandidateReady()) {
    oledLine(0,"INSTALAR NA S3?");oledLine(1,"OK:SIM Voltar:NAO");
    oledLine(2,"Atual "+s3OtaVersion);oledLine(3,"Nova  "+s3ConfirmVersion);
    oledLine(4,"Vai reiniciar a S3");
    oledLine(5,String("Prazo ")+String((s3CandidateUntil-millis())/1000)+"s");
    oledLine(6,"WROOM segue ligada");
  } else if (oledPage == 5) {
    oledLine(0,"OTA S3 "+(s3OtaVersion.isEmpty()?peerVersion:s3OtaVersion)+"");
    oledLine(1,"Automatico: LIGADO");
    if(s3Busy||!s3WatchVersion.isEmpty()){
      oledLine(2,"Acompanhando a S3");oledLine(3,s3OtaRecent()?s3OtaStatus:"Aguardando relato S3");
      oledLine(4,s3Expected?(s3OtaRecent()?String(""):String("Ultimo "))+String((unsigned long)(s3Written*100/s3Expected))+"% da S3":"Sem progresso atual");
      oledLine(5,s3WatchVersion.isEmpty()?"Automatico: LIGADO":"Destino "+s3WatchVersion);
      oledLine(6,"Nao desligue a S3");
    }else{
      const char *actions[]={"Consultar GitHub","Instalar nova","Cancelar pedido"};
      bool active[]={peerKey.length()==64&&WiFi.status()==WL_CONNECTED,s3CandidateReady(),s3OtaRecent()&&s3WindowUntil&&(int32_t)(s3WindowUntil-millis())>0};
      for(uint8_t i=0;i<3;++i)oledLine(i+2,String(i==oledS3Choice?"> ":"  ")+actions[i]+(active[i]?"":" [X]"));
      oledLine(5,s3CandidateReady()?"Nova "+s3Target+" Auto:ON":s3OtaStatus);
      oledLine(6,peerKey.length()!=64?"Pareamento pendente":!s3OtaRecent()?"Aguardando relato S3":oledS3Choice==0?"Busca; nao instala":oledS3Choice==1?"OK: abre confirmacao":"Auto retoma em 5 min");
    }
  }else{
    oledLine(0,"AJUDA - NAVEGACAO");oledLine(1,"Mesmo modelo em tudo");
    oledLine(2,"Cima: item anterior");oledLine(3,"Baixo: proximo item");
    oledLine(4,"Direita: entra / OK");oledLine(5,"Esquerda: volta nivel");oledLine(6,"[X]: indisponivel");
  }
  oled.display();
  if(!oledPresent()){oledReady=false;oledStatus="PAUSED_I2C_ERROR";diagnosticScope.failed=true;}
}
static void beginOled() {
  // I2C timeout bounds each transaction; no reset or flash writes on failure.
  if (!Wire.begin(21, 22, 400000)) {
    oledStatus = "I2C_INIT_FAILED";
  } else {
    Wire.setTimeOut(20);
    if (!oledPresent()) oledStatus = "NOT_FOUND_0x3C";
    else if (ESP.getFreeHeap() < 82000) oledStatus = "SKIPPED_LOW_HEAP";
    else if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) oledStatus = "INIT_FAILED";
    else { oledReady = true; oledStatus = "READY_128x64_SDA21_SCL22"; }
  }
  Serial.println("OLED: " + oledStatus);
  if (oledReady) {
    oled.setTextSize(1); oled.setTextColor(SSD1306_WHITE); oled.setTextWrap(false);
    refreshOled();
  }
}

// OTA runs sequentially on loopTask. Bounded OLED writes only at progress checkpoints.
void showOtaProgress(const String &phase, size_t done, size_t total) {
  if (!oledReady) return;
  if (!oledPresent()) { oledReady = false; oledStatus = "PAUSED_I2C_ERROR"; return; }
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
