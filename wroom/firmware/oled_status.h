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
  oled.setCursor(0, row * 9);
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
    const char *areas[] = {"Conexao", "Placa WROOM", "Placa S3", "Atualizacao WROOM", "Ajuda dos botoes"};
    for (uint8_t i = 0; i < 5; ++i) oledLine(i + 1, String(i == oledMenuChoice ? "> " : "  ") + areas[i]);
    oledLine(6, "Cima/Baixo OK:abrir");
  } else if (oledPage == 3) {
    oledLine(0, "CONEXAO");
    oledLine(1, WiFi.status() == WL_CONNECTED ? "Wi-Fi: conectado" : "Wi-Fi: sem conexao");
    oledLine(2, String("IP ") + WiFi.localIP().toString());
    oledLine(3, telemetryStatus == "HTTP_200" ? "Site: envio aceito" : "Site: aguardando/envio");
    oledLine(4, oledDetail ? "Site = ThingsBoard" : peerRecent() ? "S3: contato recente" : "S3: sem relato atual");
    oledLine(5, "Cima/Baixo: area");
    oledLine(6, "OK:info Voltar:menu");
  } else if (oledPage == 0) {
    oledLine(0, String("PLACA WROOM ") + BOTIZIN_VERSION);
    oledLine(1, String("Ligada ") + String(millis() / 60000) + " min");
    oledLine(2, stateInfo(esp_ota_get_running_partition()) == "VALID" ? "Programa: validado" : "Programa: " + stateInfo(esp_ota_get_running_partition()));
    oledLine(3, oledDetail ? "Reset = ultimo inicio" : resetInfo().startsWith("POWERON") ? "Inicio: ligou energia" : resetInfo().startsWith("BROWNOUT") ? "Inicio: queda energia" : resetInfo().startsWith("SOFTWARE") ? "Inicio: pelo programa" : "Inicio: " + resetInfo());
    oledLine(4, oledDetail ? "PING = resposta real" : pingStatus == "READY" ? "Comandos: prontos" : "Comandos: ver painel");
    oledLine(5, "Cima/Baixo: area");
    oledLine(6, "OK:info Voltar:menu");
  } else if (oledPage == 1) {
    oledLine(0, String("PLACA S3 ") + (peerHaveReport ? String(peerFrame.version) : ""));
    oledLine(1, peerRecent() ? "Contato: recente" : "Contato: aguardando");
    oledLine(2, peerHaveReport ? String("Relato ha ") + String((millis() - peerSeenAt) / 1000) + "s" : "Buscando na rede...");
    oledLine(3, peerHaveReport ? (oledDetail ? "Programa: " + String(peerFrame.state) : "IP " + String(peerFrame.ip)) : "Alvo: 192.168.0.36");
    oledLine(4, peerHaveReport ? String("Ligada ") + String(peerFrame.uptime / 60) + " min no relato" : "WROOM segue ativa");
    oledLine(5, "Cima/Baixo: area");
    oledLine(6, "OK:info Voltar:menu");
  } else if (oledPage == 2 && oledDetail && candidateReady()) {
    oledLine(0, "CONFIRMAR INSTALACAO?");
    oledLine(1, "Atual " + String(BOTIZIN_VERSION));
    oledLine(2, "Nova  " + otaTargetVersion);
    oledLine(3, "Vai reiniciar WROOM");
    oledLine(4, String("Prazo ") + String((otaManualUntil - millis()) / 1000) + "s");
    oledLine(5, "Cima/Baixo: opcoes");
    oledLine(6, "OK:SIM Voltar:NAO");
  } else if (oledPage == 2) {
    oledLine(0, "ATUALIZACAO WROOM");
    oledLine(1, String("Atual ") + BOTIZIN_VERSION + " Auto:ON");
    oledLine(2, candidateReady() ? "Nova " + otaTargetVersion : internetStatus.startsWith("UP_TO_DATE") ? "Sem versao nova" : otaCheckQueued ? "Consulta aguardando" : "Git: consultar versao");
    oledLine(3, String("> ") + (oledOtaChoice == 0 ? "Consultar GitHub" : oledOtaChoice == 1 ? "Instalar nova versao" : "Cancelar pedido"));
    bool active = oledOtaChoice == 0 ? otaReady && !internetStopped : oledOtaChoice == 1 ? candidateReady() : manualWindowActive();
    oledLine(4, active ? (oledOtaChoice == 1 ? "OK: ver confirmacao" : "Disponivel com OK") : oledOtaChoice == 1 ? "Bloqueado: sem nova" : "Bloqueado: sem pedido");
    oledLine(5, "Cima/Baixo: opcao");
    oledLine(6, "OK:acao Voltar:menu");
  } else {
    oledLine(0, "AJUDA DOS BOTOES");
    oledLine(1, "Cima: item anterior");
    oledLine(2, "Baixo: proximo item");
    oledLine(3, "OK: acao na tela");
    oledLine(4, "Voltar: sair da area");
    oledLine(5, "Instala: pede SIM/NAO");
    oledLine(6, "Voltar: menu inicial");
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
  oledLine(0, "OTA WROOM"); oledLine(1, phase);
  if (total) {
    oledLine(2, String((unsigned long)(done * 100 / total)) + "%");
    oledLine(3, String((unsigned long)done) + " bytes");
    oledLine(4, "de " + String((unsigned long)total));
  }
  oledLine(5, "Botoes: bloqueados");
  oledLine(6, "Nao desligue a placa");
  oled.display();
}
