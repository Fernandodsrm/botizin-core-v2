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
  if (oledPage == 0 && !oledDetail) {
    oledLine(0, String("WROOM ") + BOTIZIN_VERSION + " [1/2]");
    oledLine(1, WiFi.status() == WL_CONNECTED ? "WiFi: OK" : "WiFi: AGUARDANDO");
    oledLine(2, String("IP ") + WiFi.localIP().toString());
    const esp_partition_t *run = esp_ota_get_running_partition();
    oledLine(3, String(run ? run->label : "NONE") + (otaReady ? " / OTA PRONTO" : " / OTA BLOQUEADO"));
    oledLine(4, internetStatus.startsWith("UP_TO_DATE") ? "Internet OTA: ATUAL" : "OTA " + internetStatus);
    oledLine(5, String("TB ") + telemetryStatus);
    oledLine(6, "BAIXO:S3 OK:DETALHE");
  } else if (oledPage == 0) {
    oledLine(0, String("WROOM ") + BOTIZIN_VERSION + " DET");
    oledLine(1, String("PING ") + pingStatus);
    oledLine(2, String("Reset ") + resetInfo());
    oledLine(3, String("OTA ") + stateInfo(esp_ota_get_running_partition()));
    oledLine(4, String("Ligada ") + String(millis() / 1000) + "s");
    oledLine(5, "S3 " + peerStatus);
    oledLine(6, "OK:STATUS BAIXO:S3");
  } else if (!peerHaveReport) {
    oledLine(0, "S3 [2/2]");
    oledLine(1, "SEM RELATO DA S3");
    oledLine(2, peerStatus);
    oledLine(3, "Alvo 192.168.0.36");
    oledLine(4, "Consulta pela rede");
    oledLine(5, "WROOM segue ativa");
    oledLine(6, "VOLTAR:WROOM");
  } else {
    oledLine(0, String("S3 ") + peerFrame.version + " [2/2]");
    oledLine(1, peerRecent() ? "CONTATO RECENTE" : "SEM CONTATO RECENTE");
    oledLine(2, String("Relato ha ") + String((millis() - peerSeenAt) / 1000) + "s");
    if (!oledDetail) {
      oledLine(3, String("IP ") + peerFrame.ip);
      oledLine(4, String("OTA ") + peerFrame.state);
      oledLine(5, String("Ligada ") + String(peerFrame.uptime) + "s no relato");
    } else {
      oledLine(3, String(peerFrame.running).substring(0, 12));
      oledLine(4, String("Reset ") + peerFrame.reset);
      oledLine(5, String("OTA ") + peerFrame.internet);
    }
    oledLine(6, "OK:DET VOLTAR:WROOM");
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
