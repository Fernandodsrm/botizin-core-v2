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

static void refreshOled() {
  if (!oledReady || uploadActive || uploadOK || rebootScheduled ||
      (int32_t)(millis() - nextOledRefresh) < 0) return;
  nextOledRefresh = millis() + 1000;
  if (!oledPresent()) {
    oledReady = false;
    oledStatus = "PAUSED_I2C_ERROR";
    Serial.println("OLED: " + oledStatus);
    return;
  }
  oled.clearDisplay();
  if (oledMenu) {
    oledLine(0, "BOTIZIN - MENU");
    oledLine(1, "Cima/Baixo OK:abrir");
    const char *areas[] = {"Conexao", "Placa WROOM", "Placa S3", "Atualizacao WROOM", "Atualizacao S3", "Ajuda dos botoes"};
    uint8_t first=oledMenuChoice>=5?1:0;
    for (uint8_t i = first; i < first+5; ++i) oledLine(i-first+2, String(i == oledMenuChoice ? "> " : "  ") + areas[i]);
  } else if (oledPage == 3) {
    oledLine(0, "CONEXAO       ^v:area");
    oledLine(1, "OK:info Voltar:menu");
    oledLine(2, WiFi.status() == WL_CONNECTED ? "Wi-Fi: conectado" : "Wi-Fi: sem conexao");
    oledLine(3, String("IP ") + WiFi.localIP().toString());
    oledLine(4, telemetryStatus == "HTTP_200" ? "Site: envio aceito" : "Site: aguardando");
    oledLine(5, peerRecent() ? "S3: contato recente" : "S3: sem relato atual");
    oledLine(6, oledDetail ? "Site = ThingsBoard" : "Dados desta WROOM");
  } else if (oledPage == 0) {
    oledLine(0, String("WROOM ") + BOTIZIN_VERSION + " ^v:area");
    oledLine(1, "OK:info Voltar:menu");
    oledLine(2, String("Ligada ") + String(millis() / 60000) + " min");
    oledLine(3, stateInfo(esp_ota_get_running_partition()) == "VALID" ? "Programa: validado" : "Programa: " + stateInfo(esp_ota_get_running_partition()));
    oledLine(4, oledDetail ? "Reset = ultimo inicio" : resetInfo().startsWith("POWERON") ? "Inicio: ligou energia" : resetInfo().startsWith("BROWNOUT") ? "Inicio: queda energia" : resetInfo().startsWith("SOFTWARE") ? "Inicio: pelo programa" : "Inicio: " + resetInfo());
    oledLine(5, oledDetail ? "PING = resposta real" : pingStatus == "READY" ? "Comandos: prontos" : "Comandos: ver painel");
    oledLine(6, peerRecent() ? "S3: contato recente" : "S3: sem relato atual");
  } else if (oledPage == 1) {
    oledLine(0, String("S3 ") + (peerHaveReport ? String(peerFrame.version) : "--") + " ^v:area");
    oledLine(1, "OK:info Voltar:menu");
    oledLine(2, peerRecent() ? "Contato: recente" : "Contato: aguardando");
    oledLine(3, peerHaveReport ? String("Relato ha ") + String((millis() - peerSeenAt) / 1000) + "s" : "Buscando na rede...");
    oledLine(4, peerHaveReport ? (oledDetail ? "Programa: " + String(peerFrame.state) : "IP " + String(peerFrame.ip)) : "Alvo: 192.168.0.36");
    oledLine(5, peerHaveReport ? String("Ligada ") + String(peerFrame.uptime / 60) + " min" : "WROOM segue ativa");
    oledLine(6, "Valores do relato S3");
  } else if (oledPage == 2 && oledDetail && candidateReady()) {
    oledLine(0, "INSTALAR NA WROOM?");
    oledLine(1, "OK:SIM Voltar:NAO");
    oledLine(2, "Atual " + String(BOTIZIN_VERSION));
    oledLine(3, "Nova  " + otaTargetVersion);
    oledLine(4, "Vai reiniciar WROOM");
    oledLine(5, String("Prazo ") + String((otaManualUntil - millis()) / 1000) + "s");
    oledLine(6, "Automatico: LIGADO");
  } else if (oledPage == 2) {
    oledLine(0, String("OTA WROOM ") + BOTIZIN_VERSION + " ^v");
    oledLine(1, "OK:acao Voltar:menu");
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
    oledLine(0,"OTA S3 "+(s3OtaVersion.isEmpty()?peerVersion:s3OtaVersion)+" ^v");
    oledLine(1,"OK:acao Voltar:menu");
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
  } else {
    oledLine(0, "AJUDA DOS BOTOES");
    oledLine(1, "Voltar:menu ^v:area");
    oledLine(2, "Cima: item anterior");
    oledLine(3, "Baixo: proximo item");
    oledLine(4, "OK: acao na tela");
    oledLine(5, "^v = Cima/Baixo");
    oledLine(6, "[X] = indisponivel");
  }
  oled.display();
  if (!oledPresent()) {
    oledReady = false;
    oledStatus = "PAUSED_I2C_ERROR";
    Serial.println("OLED: " + oledStatus);
  }
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
