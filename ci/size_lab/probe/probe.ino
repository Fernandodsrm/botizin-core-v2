// SOMENTE DIMENSIONAMENTO. Não é o Menu BOTIZIN e não deve ser gravado.
// O ramo de teste é mantido pelo compilador, mas nunca ativado neste sketch.
#include <Arduino.h>
#include "features.h"
#if LAB_JSON
#include <cJSON.h>
#endif
#if LAB_RETURN
#include <esp_ota_ops.h>
#include <esp_system.h>
#endif
#if LAB_WIFI
#include <WiFi.h>
#endif
#if LAB_TLS
#include <NetworkClientSecure.h>
#endif
#if LAB_HTTP
#include <HTTPClient.h>
#endif
#if LAB_OTA
#include <Update.h>
#include <mbedtls/sha256.h>
#endif
#if LAB_NVS
#include <Preferences.h>
#endif
#if LAB_WEB
#include <WebServer.h>
WebServer labServer(80);
#endif
#if LAB_OLED
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 labDisplay(128, 64, &Wire, -1);
#endif
volatile bool labRun = false;
volatile unsigned labResult = 0;
#if LAB_MENU
const char *menuItems[] = {"Modulos", "Conexoes", "Diagnostico", "Ajuda"};
unsigned menuIndex = 0;
const uint8_t menuButtons[] = {25, 27, 32, 33};
#endif

void setup() {
  Serial.begin(115200);
  Serial.println("SIZE_PROBE_NOT_DEPLOYABLE");
  if (!labRun) return;
#if LAB_MENU
  for (auto pin : menuButtons) pinMode(pin, INPUT_PULLUP);
#endif
#if LAB_WIFI
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  labResult = WiFi.status();
#endif
#if LAB_NVS
  Preferences p;
  if (p.begin("lab-size", false)) {
    labResult += p.getString("module", "menu").length();
    labResult += p.putString("module", "menu");
    labResult += p.getString("ssid", "").length();
    labResult += p.putString("ssid", "");
    p.end();
  }
#endif
#if LAB_OLED
  Wire.begin();
  labDisplay.begin(SSD1306_SWITCHCAPVCC, 0x3c);
  labDisplay.clearDisplay(); labDisplay.setTextSize(1);
  labDisplay.setTextColor(SSD1306_WHITE); labDisplay.setCursor(0, 0);
  labDisplay.print("Menu BOTIZIN"); labDisplay.display();
#endif
#if LAB_WEB
  labServer.on("/status", []() { labServer.send(200, "text/plain", "SIZE_PROBE"); });
  labServer.begin();
#endif
}

void loop() {
  if (!labRun) { delay(100); return; }
#if LAB_MENU
  if (!digitalRead(33)) menuIndex = (menuIndex + 1) % 4;
  labDisplay.clearDisplay(); labDisplay.setCursor(0, 0);
  labDisplay.print("BOTIZIN MENU WROOM");
  for (unsigned i = 0; i < 4; ++i) {
    labDisplay.setCursor(0, 18 + i * 9);
    labDisplay.print(i == menuIndex ? "> " : "  "); labDisplay.print(menuItems[i]);
  }
  labDisplay.display();
#endif
#if LAB_JSON
  cJSON *catalog = cJSON_Parse("{\"modules\":[{\"module_id\":\"led\",\"version\":\"1.0.0\"}]}");
  cJSON *list = cJSON_GetObjectItemCaseSensitive(catalog, "modules");
  labResult += cJSON_GetArraySize(list);
  cJSON_Delete(catalog);
#endif
#if LAB_RETURN
  const esp_partition_t *running = esp_ota_get_running_partition();
  const esp_partition_t *other = esp_ota_get_next_update_partition(running);
  // Reachability only; this is NOT a validated Menu selection policy.
  if (other) {
    labResult += esp_ota_set_boot_partition(other);
    labResult += esp_ota_get_boot_partition() == other;
    esp_restart();
  }
#endif
#if LAB_TLS
  NetworkClientSecure tls;
  tls.useBuiltinCACertBundle();
#if !LAB_HTTP
  labResult += tls.connect("example.com", 443); tls.stop();
#endif
#endif
#if LAB_HTTP
  HTTPClient http;
#if LAB_TLS
  http.begin(tls, "https://example.com/");
#else
  NetworkClient net;
  http.begin(net, "http://example.com/");
#endif
  labResult += http.GET();
  String body = http.getString(); labResult += body.length();
#if LAB_CLOUD
  http.addHeader("Content-Type", "application/json");
  labResult += http.POST("{\"origin\":\"SIZE_PROBE\"}");
#endif
  http.end();
#endif
#if LAB_PEER
  NetworkClient peer;
  HTTPClient request;
  request.begin(peer, "http://127.0.0.1/status");
  labResult += request.GET(); labResult += request.getString().length(); request.end();
#endif
#if LAB_OTA
  // Unreachable sizing branch retains the write APIs; never deploy this probe.
  labResult += Update.begin(4096, U_FLASH);
  uint8_t chunk[64] = {};
  labResult += Update.write(chunk, sizeof(chunk));
  labResult += Update.end(false);
  Update.abort();
  unsigned char digest[32];
  mbedtls_sha256_context hash;
  mbedtls_sha256_init(&hash); mbedtls_sha256_starts(&hash, 0);
  mbedtls_sha256_update(&hash, (const unsigned char *)"probe", 5);
  mbedtls_sha256_finish(&hash, digest); mbedtls_sha256_free(&hash);
  labResult += digest[0];
#endif
#if LAB_WEB
  labServer.handleClient();
#endif
  delay(100);
}
