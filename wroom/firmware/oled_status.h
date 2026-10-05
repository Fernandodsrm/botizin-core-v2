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
  oledLine(0, String("BOTIZIN WROOM ") + BOTIZIN_VERSION);
  oledLine(1, WiFi.status() == WL_CONNECTED ? "WiFi: OK" : "WiFi: AGUARDANDO");
  oledLine(2, String("IP ") + WiFi.localIP().toString());
  const esp_partition_t *run = esp_ota_get_running_partition();
  oledLine(3, String(run ? run->label : "NONE") + (otaReady ? " / OTA PRONTO" : " / OTA BLOQUEADO"));
  oledLine(4, internetStatus.startsWith("UP_TO_DATE") ? "Internet OTA: ATUAL" : "OTA " + internetStatus);
  oledLine(5, String("TB ") + telemetryStatus);
  oledLine(6, String("PING ") + pingStatus);
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
