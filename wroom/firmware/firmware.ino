#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <Preferences.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <esp_partition.h>
#include <mbedtls/sha256.h>
#include <NetworkClientSecure.h>
#include <HTTPClient.h>
#include <cJSON.h>
#include <time.h>
#include "internet_config.h"
#include "version.h"
#include "provisioning.h"

WebServer server(80);
Preferences journal;
String attempt;
String priorAttempt;
String expectedSha;
String receivedSha;
String storedSha;
size_t expectedBytes = 0, writtenBytes = 0;
const esp_partition_t *destination = nullptr;
const esp_partition_t *originalBoot = nullptr;
mbedtls_sha256_context hashContext;
bool hashActive = false, uploadActive = false, uploadOK = false;
bool otaReady = false, rebootScheduled = false;
uint32_t rebootAt = 0;
String internetStatus = "WAITING_FOR_FIRST_CHECK";
bool internetStopped = false;
uint32_t nextInternetCheck = 0;
int responseCode = 400;
String telemetryStatus = "WAITING";
String oledStatus = "WAITING";
String peerStatus = "WAITING", peerVersion;
uint32_t peerSeenAt = 0;
uint8_t oledPage = 0;
bool oledDetail = false, oledMenu = true;
uint8_t oledMenuChoice = 0, oledOtaChoice = 0;
uint32_t nextTelemetry = 0, telemetrySequence = 0;
uint8_t telemetryFailures = 0;
bool telemetryStopped = false;
String telemetryBootId;
#include "diagnostics.h"
// Automatic OTA remains enabled. A manual query reserves a bounded confirmation window.
String manualOtaStatus = "READY", otaCandidateId, otaTargetVersion, otaTargetURL, otaTargetSHA;
size_t otaTargetBytes = 0;
uint32_t otaManualUntil = 0;
bool otaCheckQueued = false, otaInstallQueued = false;
void showOtaProgress(const String &phase, size_t done = 0, size_t total = 0);
#include "ota_confirmation.h"
#include "peer_auth.h"
static void discardNavigation();
#include "navigation_model.h"
#include "peer_ota_client.h"

bool manualWindowActive() { return otaManualUntil && (int32_t)(otaManualUntil - millis()) > 0; }
bool candidateReady() { return !otaCandidateId.isEmpty() && manualWindowActive() && !otaInstallQueued && !uploadActive && !uploadOK && !rebootScheduled; }
void clearOtaCandidate() { otaCandidateId = ""; otaTargetVersion = ""; otaTargetURL = ""; otaTargetSHA = ""; otaTargetBytes = 0; }
bool queueOtaCheck() {
  if (!otaReady || internetStopped || otaCheckQueued || otaInstallQueued || uploadActive || uploadOK || rebootScheduled) return false;
  clearOtaCandidate(); otaManualUntil = millis() + 300000;
  otaCheckQueued = true; manualOtaStatus = "CHECK_QUEUED"; return true;
}
bool confirmOta(const String &id, const String &version, const String &sha, const String &boot) {
  if (!candidateReady() || !otaConfirmationMatches(millis(), otaManualUntil, id.c_str(), otaCandidateId.c_str(), version.c_str(), otaTargetVersion.c_str(), sha.c_str(), otaTargetSHA.c_str(), boot.c_str(), telemetryBootId.c_str())) return false;
  otaInstallQueued = true; manualOtaStatus = "INSTALL_QUEUED"; return true;
}
void cancelOtaCheck() {
  if (otaInstallQueued || uploadActive || uploadOK || rebootScheduled) return;
  otaCheckQueued = false; clearOtaCandidate();
  // Cancellation grants five minutes to the user before automatic installation resumes.
  manualOtaStatus = "CANCELLED_AUTO_IN_5MIN"; otaManualUntil = millis() + 300000;
}


String partitionInfo(const esp_partition_t *p) {
  if (!p) return "NONE";
  char b[128];
  String subtype = "other";
  if (p->type == ESP_PARTITION_TYPE_APP && p->subtype >= ESP_PARTITION_SUBTYPE_APP_OTA_MIN &&
      p->subtype <= ESP_PARTITION_SUBTYPE_APP_OTA_MAX) {
    subtype = "ota_" + String(p->subtype - ESP_PARTITION_SUBTYPE_APP_OTA_MIN);
  }
  snprintf(b, sizeof(b), "%s / %s / address=0x%08lx / size=%lu", p->label, subtype.c_str(),
           (unsigned long)p->address, (unsigned long)p->size);
  return String(b);
}

String stateInfo(const esp_partition_t *p) {
  if (!p) return "NONE";
  esp_ota_img_states_t state;
  esp_err_t e = esp_ota_get_state_partition(p, &state);
  if (e != ESP_OK) return String(esp_err_to_name(e)) + " (state unavailable)";
  switch (state) {
    case ESP_OTA_IMG_NEW: return "NEW";
    case ESP_OTA_IMG_PENDING_VERIFY: return "PENDING_VERIFY";
    case ESP_OTA_IMG_VALID: return "VALID";
    case ESP_OTA_IMG_INVALID: return "INVALID";
    case ESP_OTA_IMG_ABORTED: return "ABORTED";
    case ESP_OTA_IMG_UNDEFINED: return "UNDEFINED";
    default: return String((int)state);
  }
}

String resetInfo() {
  esp_reset_reason_t r = esp_reset_reason();
  const char *name = "OTHER";
  switch (r) {
    case ESP_RST_POWERON: name = "POWERON"; break;
    case ESP_RST_SW: name = "SOFTWARE"; break;
    case ESP_RST_PANIC: name = "PANIC"; break;
    case ESP_RST_INT_WDT: name = "INT_WDT"; break;
    case ESP_RST_TASK_WDT: name = "TASK_WDT"; break;
    case ESP_RST_WDT: name = "WDT"; break;
    case ESP_RST_BROWNOUT: name = "BROWNOUT"; break;
    case ESP_RST_EXT: name = "EXTERNAL"; break;
    case ESP_RST_DEEPSLEEP: name = "DEEPSLEEP"; break;
    default: break;
  }
  return String(name) + " (" + String((int)r) + ")";
}

String snapshot() {
  const esp_partition_t *run = esp_ota_get_running_partition();
  String s = "BOTIZIN WROOM V" + String(BOTIZIN_VERSION) + "\n";
  s += "IP: " + WiFi.localIP().toString() + "\n";
  s += "UPTIME_SECONDS: " + String(diagnosticUptime()) + "\n";
  s += "RESET_REASON: " + resetInfo() + "\n";
  s += "FLASH_BYTES: " + String(ESP.getFlashChipSize()) + "\n";
  s += "PSRAM_BYTES: " + String(ESP.getPsramSize()) + "\n";
  s += "PSRAM_HEAP_BYTES: " + String(ESP.getPsramSize()) + "\n";
  s += "PSRAM_FOUND: " + String(psramFound() ? "YES" : "NO") + "\n";
  s += "CPU_MHZ: " + String(ESP.getCpuFreqMHz()) + "\n";
  s += "RUNNING_PARTITION: " + partitionInfo(run) + "\n";
  s += "BOOT_PARTITION: " + partitionInfo(esp_ota_get_boot_partition()) + "\n";
  s += "NEXT_UPDATE_PARTITION: " + partitionInfo(esp_ota_get_next_update_partition(nullptr)) + "\n";
  s += "RUNNING_OTA_STATE: " + stateInfo(run) + "\n";
#ifdef CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
  s += "ROLLBACK_CONFIG: ENABLED (Arduino core default confirms the image at startup)\n";
#else
  s += "ROLLBACK_CONFIG: DISABLED\n";
#endif
  s += "WIFI: " + String(WiFi.status() == WL_CONNECTED ? "OK" : "DISCONNECTED") + "\n";
  s += "OTA: " + String(otaReady ? "READY" : "BLOCKED: hardware/partition mismatch") + "\n";
  s += "INTERNET_OTA: " + internetStatus + "\n";
  s += "OTA_AUTOMATIC_ENABLED: YES\nOTA_MANUAL_STATUS: " + manualOtaStatus + "\n";
  s += "MANIFEST_URL: " + String(BOTIZIN_MANIFEST_URL) + "\n";
  s += "CHECK_INTERVAL_SECONDS: 60\n";
  s += "TELEMETRY: " + telemetryStatus + "\n";
  s += "OLED: " + oledStatus + "\n";
  s += "OLED_PAGE: " + String(oledMenu ? "MENU" : navLevel == 1 ? "BOARD_LIST" : navLevel == 2 ? "DIAGNOSTIC_LIST" : oledPage == 7 ? "DIAGNOSTICS" : oledPage == 8 ? "CONNECTION" : oledPage == 9 ? "CONTROLS" : oledPage == 2 ? "OTA_WROOM" : oledPage == 5 ? "OTA_S3" : "HELP") + "\n";
  s += "S3_LINK: " + peerStatus + "\n";
  s += "S3_EXPECTED_URL: http://192.168.0.36/status\n";
  s += "S3_LAST_VERSION: " + peerVersion + "\n";
  s += "PEER_PAIRED: " + String(peerKey.length()==64?"YES":"NO") + "\n";
  s += "S3_OTA_STATUS: " + s3OtaStatus + "\n";
  s += "S3_OTA_VERSION: " + s3OtaVersion + "\n";
  s += diagnosticText();
  if (priorAttempt.length()) s += "\nLAST_PERSISTED_OTA_ATTEMPT:\n" + priorAttempt;
  if (attempt.length()) s += "\nCURRENT_OTA_ATTEMPT:\n" + attempt;
  return s;
}

void logLine(const String &s) {
  Serial.println(s);
  attempt += s + "\n";
}

void checkpoint() {
  size_t n = journal.putString("last", attempt);
  if (n == 0) Serial.println("JOURNAL_WRITE_FAILED");
}

String hexDigest(const unsigned char *bytes) {
  char out[65];
  for (int i = 0; i < 32; ++i) snprintf(out + 2 * i, 3, "%02x", bytes[i]);
  out[64] = 0;
  return String(out);
}

bool hashPartition(const esp_partition_t *p, size_t size, String &result) {
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  bool ok = mbedtls_sha256_starts(&ctx, 0) == 0;
  unsigned char block[1024], digest[32];
  for (size_t offset = 0; ok && offset < size; offset += sizeof(block)) {
    size_t count = min(sizeof(block), size - offset);
    ok = esp_partition_read(p, offset, block, count) == ESP_OK &&
         mbedtls_sha256_update(&ctx, block, count) == 0;
    yield();
  }
  if (ok) ok = mbedtls_sha256_finish(&ctx, digest) == 0;
  mbedtls_sha256_free(&ctx);
  if (ok) result = hexDigest(digest);
  return ok;
}

void failUpload(const String &reason) {
  logLine("FAIL: " + reason);
  logLine("UPDATE_LIBRARY_ERROR: " + String(Update.getError()) + " / " + Update.errorString());
  if (Update.isRunning()) Update.abort();
  if (hashActive) { mbedtls_sha256_free(&hashContext); hashActive = false; }
  uploadActive = false;
  uploadOK = false;
  logLine("BYTES_WRITTEN: " + String(writtenBytes));
  logLine("BOOT_PARTITION_AFTER_FAILURE: " + partitionInfo(esp_ota_get_boot_partition()));
  checkpoint();
}

void beginUpload(const String &length, const String &sha) {
    uploadOK = false;
    responseCode = 400;
    attempt = "";
    writtenBytes = 0;
    receivedSha = storedSha = "";
    destination = esp_ota_get_next_update_partition(nullptr);
    originalBoot = esp_ota_get_boot_partition();
    logLine("SOURCE_VERSION: " + String(BOTIZIN_VERSION));
    logLine("RUNNING_PARTITION_BEFORE: " + partitionInfo(esp_ota_get_running_partition()));
    logLine("BOOT_PARTITION_BEFORE: " + partitionInfo(originalBoot));
    logLine("NEXT_UPDATE_PARTITION_BEFORE: " + partitionInfo(destination));
    logLine("RESET_REASON: " + resetInfo());
    expectedSha = sha; expectedSha.toLowerCase();
    
    bool digits = length.length() > 0 && length.length() <= 8;
    for (size_t i = 0; i < length.length(); ++i) digits &= isDigit(length[i]);
    expectedBytes = digits ? strtoul(length.c_str(), nullptr, 10) : 0;
    bool validSha = expectedSha.length() == 64;
    for (size_t i = 0; i < expectedSha.length(); ++i) validSha = validSha && (isxdigit((unsigned char)expectedSha[i]) != 0);
    logLine("EXPECTED_BYTES: " + String(expectedBytes));
    logLine("SHA_EXPECTED: " + expectedSha);
    if (!otaReady || rebootScheduled || !destination || !validSha || expectedBytes == 0 ||
        expectedBytes > destination->size) {
      failUpload("invalid metadata, hardware, partitions or pending reboot; Update.begin NOT CALLED"); return;
    }
    bool beginOK = Update.begin(expectedBytes, U_FLASH);
    logLine("Update.begin: " + String(beginOK ? "TRUE" : "FALSE"));
    if (!beginOK) { failUpload("Update.begin rejected image size/partition"); return; }
    bool shaAccepted = Update.setSHA256(expectedSha.c_str());
    logLine("Update.setSHA256: " + String(shaAccepted ? "TRUE" : "FALSE"));
    if (!shaAccepted) { failUpload("Update SHA configuration rejected"); return; }
    mbedtls_sha256_init(&hashContext); hashActive = true;
    if (mbedtls_sha256_starts(&hashContext, 0) != 0) { failUpload("SHA init failed"); return; }
    uploadActive = true;
    checkpoint();
  }

void writeUpload(uint8_t *buf, size_t count) {
    if (count > expectedBytes - writtenBytes) { failUpload("body exceeds expected size"); return; }
    if (mbedtls_sha256_update(&hashContext, buf, count) != 0) { failUpload("SHA update failed"); return; }
    size_t n = Update.write(buf, count);
    writtenBytes += n;
    if (n != count) failUpload("short Update.write");
  }

void finishUpload() {
    unsigned char digest[32];
    int hashResult = mbedtls_sha256_finish(&hashContext, digest);
    mbedtls_sha256_free(&hashContext); hashActive = false;
    if (hashResult != 0) { failUpload("SHA finish failed"); return; }
    receivedSha = hexDigest(digest);
    logLine("BYTES_WRITTEN: " + String(writtenBytes));
    logLine("SHA_CALCULATED_RECEIVED: " + receivedSha);
    if (writtenBytes != expectedBytes || receivedSha != expectedSha) {
      logLine("Update.end: NOT CALLED"); failUpload("size/SHA mismatch"); return;
    }
    // Arduino Update.end(false) validates/activates the written image; no extra set_boot call.
    bool endOK = Update.end(false);
    logLine("Update.end: " + String(endOK ? "TRUE" : "FALSE"));
    logLine("UPDATE_LIBRARY_ERROR: " + String(Update.getError()) + " / " + Update.errorString());
    if (!endOK) { failUpload("Update.end failed"); return; }
    bool readOK = hashPartition(destination, expectedBytes, storedSha);
    logLine("SHA_CALCULATED_FLASH: " + (readOK ? storedSha : String("READ_ERROR")));
    const esp_partition_t *boot = esp_ota_get_boot_partition();
    logLine("RUNNING_PARTITION_AFTER_WRITE: " + partitionInfo(esp_ota_get_running_partition()));
    logLine("BOOT_PARTITION_AFTER_Update.end: " + partitionInfo(boot));
    logLine("DESTINATION_OTA_STATE: " + stateInfo(destination));
    esp_app_desc_t desc;
    esp_err_t descResult = esp_ota_get_partition_description(destination, &desc);
    logLine("IMAGE_DESCRIPTION: " + String(esp_err_to_name(descResult)));
    bool valid = readOK && storedSha == expectedSha && boot && boot->address == destination->address && descResult == ESP_OK;
    if (!valid) {
      // Restore the original boot target only if post-write verification fails. Stay here for investigation.
      esp_err_t restore = esp_ota_set_boot_partition(originalBoot);
      logLine("RESTORE_ORIGINAL_BOOT: " + String(esp_err_to_name(restore)));
      failUpload("post-write verification failed; STOP, no automatic reboot"); return;
    }
    uploadActive = false;
    uploadOK = true;
    responseCode = 200;
    logLine("OTA_WRITE_VERIFIED: YES; reboot requires POST /reboot");
    checkpoint();
  }

void handleUpload() {
  HTTPUpload &u = server.upload();
  if (u.status == UPLOAD_FILE_START) beginUpload(server.arg("size"), server.arg("sha256"));
  else if (u.status == UPLOAD_FILE_WRITE && uploadActive) writeUpload(u.buf, u.currentSize);
  else if (u.status == UPLOAD_FILE_END && uploadActive) finishUpload();
  else if (u.status == UPLOAD_FILE_ABORTED && uploadActive) failUpload("HTTP upload aborted; Update.end NOT CALLED");
}

bool newerVersion(const String &candidate) {
  unsigned int a, b, c, x, y, z;
  char tail;
  if (sscanf(candidate.c_str(), "%u.%u.%u%c", &a, &b, &c, &tail) != 3 ||
      sscanf(BOTIZIN_VERSION, "%u.%u.%u%c", &x, &y, &z, &tail) != 3) return false;
  return a > x || (a == x && (b > y || (b == y && c > z)));
}

void stopInternet(const String &reason) {
  internetStatus = "STOPPED: " + reason;
  internetStopped = true;
  Serial.println("INTERNET_OTA: " + internetStatus);
}

void checkInternetOTA(bool manual) {
  DiagnosticScope diagnosticScope(DIAG_GIT);
  if (manual) { manualOtaStatus = "CHECKING"; showOtaProgress("CONSULTANDO GITHUB"); }
  if (!otaReady) { stopInternet("hardware/partition mismatch"); return; }
  if (WiFi.status() != WL_CONNECTED) {
    internetStatus = "WAITING_FOR_WIFI"; return;
  }
  // TLS certificate validity requires a clock synchronized by SNTP.
  if (time(nullptr) < 1700000000) {
    internetStatus = "WAITING_FOR_NTP";
    Serial.println("INTERNET_OTA: " + internetStatus); return;
  }
  Serial.println("INTERNET_OTA: CHECKING_MANIFEST");
  NetworkClientSecure tls;
  tls.useBuiltinCACertBundle();
  tls.setHandshakeTimeout(15);
  HTTPClient http;
  http.setConnectTimeout(15000);
  http.setTimeout(15000);
  http.useHTTP10(true);
  if (!http.begin(tls, BOTIZIN_MANIFEST_URL)) {
    diagnosticScope.failed=true;internetStatus = "MANIFEST_BEGIN_FAILED"; return;
  }
  http.addHeader("Cache-Control", "no-cache");
  int code = http.GET();
  Serial.printf("MANIFEST_HTTP: %d\n", code);
  int manifestSize = http.getSize();
  if (code != 200) {
    diagnosticScope.failed=true;
    internetStatus = "MANIFEST_HTTP_" + String(code);
    http.end(); return;
  }
  if (manifestSize <= 0 || manifestSize > 2048) {
    http.end(); stopInternet("invalid manifest size"); return;
  }
  String body = http.getString();
  http.end();
  if (body.length() != (size_t)manifestSize) {
    stopInternet("incomplete manifest"); return;
  }
  cJSON *root = cJSON_Parse(body.c_str());
  cJSON *version = cJSON_GetObjectItemCaseSensitive(root, "version");
  cJSON *url = cJSON_GetObjectItemCaseSensitive(root, "url");
  cJSON *sha = cJSON_GetObjectItemCaseSensitive(root, "sha256");
  cJSON *size = cJSON_GetObjectItemCaseSensitive(root, "size");
  cJSON *board = cJSON_GetObjectItemCaseSensitive(root, "board");
  bool valid = cJSON_IsObject(root) && cJSON_IsString(version) &&
      cJSON_IsString(url) && cJSON_IsString(sha) && cJSON_IsNumber(size) &&
      cJSON_IsString(board) && String(board->valuestring) == "esp32-wroom-4mb" &&
      size->valuedouble >= 1 && size->valuedouble <= 1310720 &&
      size->valuedouble == (double)size->valueint;
  if (!valid) {
    cJSON_Delete(root); stopInternet("invalid manifest fields"); return;
  }
  String targetVersion(version->valuestring), targetURL(url->valuestring), targetSHA(sha->valuestring);
  size_t targetBytes = (size_t)size->valueint;
  cJSON_Delete(root);
  // Restrict downloads to this new repository; reject plain HTTP and redirects.
  if (!targetURL.startsWith(BOTIZIN_FIRMWARE_PREFIX) || targetSHA.length() != 64) {
    stopInternet("unexpected firmware URL/SHA"); return;
  }
  for (size_t i = 0; i < targetSHA.length(); ++i) {
    if (isxdigit((unsigned char)targetSHA[i]) == 0) {
      stopInternet("invalid firmware SHA"); return;
    }
  }
  if (targetVersion == BOTIZIN_VERSION) {
    internetStatus = "UP_TO_DATE " + targetVersion;
    Serial.println("INTERNET_OTA: " + internetStatus); return;
  }
  if (!newerVersion(targetVersion)) {
    stopInternet("manifest version is invalid or older"); return;
  }
  if (manual) {
    otaTargetVersion = targetVersion; otaTargetURL = targetURL; otaTargetSHA = targetSHA; otaTargetBytes = targetBytes;
    otaCandidateId = telemetryBootId + "-" + String((unsigned long)esp_random(), HEX);
    otaManualUntil = millis() + 300000; manualOtaStatus = "AVAILABLE";
    nextTelemetry = millis(); showOtaProgress("VERSAO DISPONIVEL"); return;
  }
  installInternetOTA(targetVersion, targetURL, targetSHA, targetBytes);
}

void installInternetOTA(const String &targetVersion, const String &targetURL, const String &targetSHA, size_t targetBytes) {
  if (!otaReady || internetStopped || uploadActive || uploadOK || rebootScheduled || WiFi.status() != WL_CONNECTED || time(nullptr) < 1700000000 || !newerVersion(targetVersion)) {
    manualOtaStatus = "INSTALL_REJECTED"; return;
  }
  showOtaProgress("ABRINDO DOWNLOAD");
  Serial.println("TARGET_VERSION: " + targetVersion);
  Serial.println("DOWNLOAD_URL: " + targetURL);
  NetworkClientSecure firmwareTLS;
  firmwareTLS.useBuiltinCACertBundle();
  firmwareTLS.setHandshakeTimeout(15);
  HTTPClient download;
  download.setConnectTimeout(15000);
  download.setTimeout(15000);
  download.useHTTP10(true);
  if (!download.begin(firmwareTLS, targetURL)) {
    stopInternet("firmware HTTP begin failed"); return;
  }
  int firmwareCode = download.GET();
  Serial.printf("FIRMWARE_HTTP: %d\n", firmwareCode);
  if (firmwareCode != 200 || download.getSize() != (int)targetBytes) {
    download.end(); stopInternet("firmware HTTP/size mismatch; no flash writes"); return;
  }
  internetStatus = "DOWNLOADING " + targetVersion;
  beginUpload(String(targetBytes), targetSHA);
  logLine("TRANSPORT: HTTPS / authenticated CA bundle");
  logLine("TARGET_VERSION: " + targetVersion);
  checkpoint();
  if (!uploadActive) {
    download.end(); stopInternet("Update.begin/SHA rejected; inspect journal"); return;
  }
  NetworkClient *stream = download.getStreamPtr();
  static uint8_t buffer[4096]; // Fixed storage: do not consume loopTask stack.
  uint32_t lastData = millis(), started = millis();
  size_t nextProgress = 65536;
  while (uploadActive && writtenBytes < expectedBytes) {
    int available = stream->available();
    if (available > 0) {
      size_t count = min((size_t)available, min(sizeof(buffer), expectedBytes - writtenBytes));
      int received = stream->read(buffer, count);
      if (received > 0) {
        writeUpload(buffer, (size_t)received);
        lastData = millis();
        if (writtenBytes >= nextProgress) {
          Serial.printf("DOWNLOAD_PROGRESS: %lu/%lu\n", (unsigned long)writtenBytes, (unsigned long)expectedBytes);
          nextProgress += 65536;
          showOtaProgress("BAIXANDO", writtenBytes, expectedBytes);
        }
      }
    } else if (!download.connected()) {
      failUpload("HTTPS connection ended before expected bytes"); break;
    }
    if (millis() - lastData > 15000 || millis() - started > 180000) {
      failUpload("HTTPS download timeout"); break;
    }
    delay(1);
  }
  download.end();
  showOtaProgress("VERIFICANDO SHA256", writtenBytes, expectedBytes);
  if (uploadActive) finishUpload();
  if (!uploadOK) { stopInternet("download/write verification failed; no retry or reboot"); return; }
  internetStatus = "VERIFIED " + targetVersion + "; REBOOT_PENDING";
  manualOtaStatus = "VERIFIED_REBOOT_PENDING";
  showOtaProgress("SHA OK REINICIANDO", writtenBytes, expectedBytes);
  logLine("REBOOT_REQUESTED: SOFTWARE / INTERNET_OTA");
  checkpoint();
  rebootAt = millis() + 2000;
  rebootScheduled = true;
}

// Separate diagnostic sender: no Update, NVS write, boot change or Wi-Fi reconfiguration.
// Called sequentially AFTER the existing OTA check has returned and freed its TLS clients.
void __attribute__((noinline)) sendTelemetry() {
  if (!provisioned || telemetryStopped || rebootScheduled || uploadActive || uploadOK ||
      WiFi.status() != WL_CONNECTED || time(nullptr) < 1700000000) return;
  if (ESP.getFreeHeap() < 80000) { telemetryStatus = "SKIPPED_LOW_HEAP"; return; }
  DiagnosticScope diagnosticScope(DIAG_TB);
  cJSON *root = cJSON_CreateObject();
  if (!root) { telemetryStatus = "SKIPPED_NO_MEMORY"; return; }
  const esp_partition_t *run = esp_ota_get_running_partition();
  String runText = partitionInfo(run);
  String bootText = partitionInfo(esp_ota_get_boot_partition());
  String nextText = partitionInfo(esp_ota_get_next_update_partition(nullptr));
  String resetText = resetInfo(), stateText = stateInfo(run);
  // Preserve the persisted source version and SHA evidence without rewriting the OTA journal.
  String evidence = priorAttempt.substring(0, 4096);
  bool ok = cJSON_AddStringToObject(root, "origin", "ESP32_REAL") &&
    cJSON_AddBoolToObject(root, "simulated", false) &&
    cJSON_AddStringToObject(root, "firmware_version", BOTIZIN_VERSION) &&
    cJSON_AddStringToObject(root, "board", "esp32-wroom-4mb") &&
    cJSON_AddStringToObject(root, "device_id", WiFi.macAddress().c_str()) &&
    cJSON_AddStringToObject(root, "boot_id", telemetryBootId.c_str()) &&
    cJSON_AddNumberToObject(root, "sequence", ++telemetrySequence) &&
    cJSON_AddNumberToObject(root, "uptime_seconds", diagnosticUptime()) &&
    cJSON_AddStringToObject(root, "running_partition", runText.c_str()) &&
    cJSON_AddStringToObject(root, "boot_partition", bootText.c_str()) &&
    cJSON_AddStringToObject(root, "next_update_partition", nextText.c_str()) &&
    cJSON_AddStringToObject(root, "reset_reason", resetText.c_str()) &&
    cJSON_AddStringToObject(root, "ota_state", stateText.c_str()) &&
    cJSON_AddStringToObject(root, "ota_journal", evidence.c_str()) &&
    cJSON_AddBoolToObject(root, "ota_automatic_enabled", true) &&
    cJSON_AddBoolToObject(root, "peer_paired", peerKey.length()==64) &&
    cJSON_AddStringToObject(root, "s3_ota_status", s3OtaStatus.c_str()) &&
    cJSON_AddStringToObject(root, "s3_ota_version", s3OtaVersion.c_str()) &&
    cJSON_AddNumberToObject(root, "s3_ota_report_age_seconds", s3HaveOtaReply?double((millis()-s3ReplyAt)/1000):-1.0) &&
    cJSON_AddStringToObject(root, "ota_manual_status", manualOtaStatus.c_str()) &&
    cJSON_AddStringToObject(root, "s3_link", peerStatus.c_str()) &&
    cJSON_AddStringToObject(root, "s3_last_version", peerVersion.c_str()) &&
    cJSON_AddNumberToObject(root, "s3_last_seen_age_seconds", peerVersion.length() ? double((millis() - peerSeenAt) / 1000) : -1.0);
  ok = ok && diagnosticJSON(root);
  char *payload = ok ? cJSON_PrintUnformatted(root) : nullptr;
  cJSON_Delete(root);
  if (!payload) { telemetryStatus = "SKIPPED_NO_MEMORY"; return; }
  size_t length = strlen(payload);
  int code = -1000;
  if (length <= 8192) {
    NetworkClientSecure tls;
    tls.useBuiltinCACertBundle();
    tls.setHandshakeTimeout(5);
    HTTPClient http;
    http.setConnectTimeout(5000);
    http.setTimeout(5000);
    http.useHTTP10(true);
    http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
    // Device token only. The account API key is NEVER used by the ESP32.
    String url = "https://thingsboard.cloud/api/v1/";
    url += BOTIZIN_TB_DEVICE_TOKEN;
    url += "/telemetry";
    if (http.begin(tls, url)) {
      http.addHeader("Content-Type", "application/json");
      code = http.POST((uint8_t *)payload, length);
    }
    http.end();
    tls.stop();
  }
  diagnosticScope.failed = code != 200;
  cJSON_free(payload);
  telemetryStatus = "HTTP_" + String(code);
  Serial.println("TELEMETRY: " + telemetryStatus);
  if (code >= 200 && code < 300) telemetryFailures = 0;
  else if (++telemetryFailures >= 3) {
    telemetryStopped = true;
    telemetryStatus = "PAUSED_AFTER_3_FAILURES_UNTIL_REBOOT";
    Serial.println("TELEMETRY: " + telemetryStatus);
  }
}

#include "ping_rpc.h"
#include "peer_status.h"
#include "oled_status.h"
#include "navigation.h"

void printPartitionTable() {
  Serial.println("PARTITION_TABLE:");
  esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, nullptr);
  while (it) {
    const esp_partition_t *p = esp_partition_get(it);
    Serial.printf("label=%s type=0x%02x subtype=0x%02x address=0x%08lx size=%lu\n",
                  p->label, p->type, p->subtype, (unsigned long)p->address, (unsigned long)p->size);
    it = esp_partition_next(it);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  loadPeerKey();
  bool journalOK = journal.begin("wroom-journal", false);
  if (journalOK) priorAttempt = journal.getString("last", "");
  else Serial.println("JOURNAL_OPEN_FAILED");
  printPartitionTable();
  const esp_partition_t *a = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, nullptr);
  const esp_partition_t *b = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, nullptr);
  const esp_partition_t *data = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, nullptr);
  const esp_partition_t *run = esp_ota_get_running_partition();
  otaReady = journalOK && ESP.getFlashChipSize() == 4 * 1024 * 1024 &&
             a && b && data && a->address == 0x10000 && b->address == 0x150000 &&
             a->size == 0x140000 && b->size == 0x140000 &&
             data->address == 0xe000 && data->size == 0x2000 && run &&
             (run->address == a->address || run->address == b->address);
  sampleDiagnostics();
  Serial.println(snapshot());
  loadProvisioning();
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  if (provisioned) WiFi.begin(wroomSSID.c_str(), wroomPassword.c_str());
  uint32_t start = millis();
  while (provisioned && WiFi.status() != WL_CONNECTED && millis() - start < 30000) delay(100);
  Serial.println(snapshot());
  server.on("/", HTTP_GET, []() { server.send(200, "text/plain; charset=utf-8", snapshot()); });
  server.on("/status", HTTP_GET, []() { server.send(200, "text/plain; charset=utf-8", snapshot()); });
  server.on("/update", HTTP_POST, []() {
    if (uploadActive) failUpload("request ended before file completion");
    server.send(responseCode, "text/plain; charset=utf-8", attempt.length() ? attempt : "No firmware uploaded\n");
  }, handleUpload);
  server.on("/reboot", HTTP_POST, []() {
    if (!uploadOK) { server.send(409, "text/plain", "No verified OTA pending; STOP\n"); return; }
    logLine("REBOOT_REQUESTED: SOFTWARE"); checkpoint();
    server.send(200, "text/plain", "Reboot in 2 seconds\n");
    rebootAt = millis() + 2000; rebootScheduled = true;
  });
  configTime(0, 0, "pool.ntp.org", "time.cloudflare.com");
  telemetryBootId = String((unsigned long)esp_random(), HEX) + String((unsigned long)esp_random(), HEX);
  nextPingPoll = millis() + 30000;
  nextTelemetry = millis() + 20000;
  nextInternetCheck = millis() + 10000;
  server.begin();
  Serial.println("HTTP_SERVER: port 80; GET /status; POST /update?size=...&sha256=...");
  beginOled();
  beginNavigation();
  nextPeerPoll = millis() + 12000;
}

void loop() {
  diagnosticLoopTick();
  if (!uploadActive && !uploadOK && !rebootScheduled) sampleDiagnostics();
  pollNavigation();
  pollProvisioning(uploadActive || uploadOK || rebootScheduled);
  server.handleClient();
  if (rebootScheduled && (int32_t)(millis() - rebootAt) >= 0) { Serial.flush(); ESP.restart(); }
  if (!rebootScheduled && !uploadActive && !uploadOK && otaInstallQueued) {
    String version = otaTargetVersion, url = otaTargetURL, sha = otaTargetSHA;
    size_t bytes = otaTargetBytes;
    otaInstallQueued = false; clearOtaCandidate(); otaManualUntil = 0;
    installInternetOTA(version, url, sha, bytes);
  }
  if (!rebootScheduled && !uploadActive && !uploadOK && otaCheckQueued) {
    otaCheckQueued = false;
    checkInternetOTA(true);
    if (manualOtaStatus == "CHECKING") manualOtaStatus = internetStatus;
    // Presses made while the synchronous query was running cannot confirm its result.
    discardNavigation(); nextOledRefresh = 0; nextTelemetry = millis();
  }
  if (!rebootScheduled && !uploadActive && !uploadOK && !internetStopped &&
      (int32_t)(millis() - nextInternetCheck) >= 0) {
    nextInternetCheck = millis() + 60000;
    if (!manualWindowActive()) { clearOtaCandidate(); checkInternetOTA(false); }

  }
  if (!rebootScheduled && !uploadActive && !uploadOK && !telemetryStopped &&
      (int32_t)(millis() - nextTelemetry) >= 0) {
    nextTelemetry = millis() + 60000;
    sendTelemetry();
  }
  if (!rebootScheduled && !uploadActive && !uploadOK && !pingStopped &&
      (int32_t)(millis() - nextPingPoll) >= 0) {
    nextPingPoll = millis() + 15000;
    pollPing();
  }
  pollNavigation();
  refreshOled();
  pollPeerStatus();
  pollS3OTA();
  pollNavigation();
  refreshOled();
  delay(2);
}
