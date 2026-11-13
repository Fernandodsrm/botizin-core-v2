#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <Preferences.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <esp_partition.h>
#include <mbedtls/sha256.h>
#include <cJSON.h>
#include "internet_config.h"
#include "version.h"
#define BOTIZIN_LOCAL_NAME "botizin-wroom"
#define BOTIZIN_LOCAL_BOARD "esp32-wroom-4mb"
#include "local_name.h"
#include "provisioning.h"
#include "remote_led.h"

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
// Physical menu approval only. A22 may upload a SHA-pinned module for 120 seconds.
static bool modulePushReady=false;
static uint32_t modulePushUntil=0;
int responseCode = 400;
String telemetryStatus = "WAITING";
String oledStatus = "WAITING";
uint8_t oledPage = 0;
bool oledDetail = false, oledMenu = true;
static bool voiceLabNextBootBaseArmed=false;
uint8_t oledMenuChoice = 0, oledOtaChoice = 0;
uint32_t nextTelemetry = 0, telemetrySequence = 0;
uint8_t telemetryFailures = 0;
bool telemetryStopped = false;
String telemetryBootId;
#include "diagnostics.h"
// Automatic checks can be disabled after proving the manual cloud path.
bool otaAutomaticEnabled = true;
bool saveAutomatic(bool enabled) {
  Preferences prefs; if (!prefs.begin("ota-policy", false)) return false;
  prefs.putBool("automatic", enabled);
  bool saved=prefs.getBool("automatic", !enabled)==enabled; prefs.end();
  if(saved) otaAutomaticEnabled=enabled; return saved;
}

String manualOtaStatus = "READY", otaCandidateId, otaTargetVersion, otaTargetURL, otaTargetSHA;
size_t otaTargetBytes = 0;
uint32_t otaManualUntil = 0;
bool otaCheckQueued = false, otaInstallQueued = false;
void showOtaProgress(const String &phase, size_t done = 0, size_t total = 0);
#include "ota_confirmation.h"
static void discardNavigation();
#include "navigation_model.h"
#include "face_state.h"
#include "voice_poll_s3.h"
#include "catalog_state.h"

bool manualWindowActive() { return otaManualUntil && (int32_t)(otaManualUntil - millis()) > 0; }
bool candidateReady() { return !otaCandidateId.isEmpty() && manualWindowActive() && !otaInstallQueued && !uploadActive && !uploadOK && !rebootScheduled && !catalogRefreshQueued; }
void clearOtaCandidate() { otaEnvironment=""; otaCandidateId = ""; otaTargetVersion = ""; otaTargetURL = ""; otaTargetSHA = ""; otaTargetBytes = 0; }
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
  manualOtaStatus = otaAutomaticEnabled ? "CANCELLED_AUTO_IN_5MIN" : "CANCELLED"; otaManualUntil = millis() + 300000;
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
  s += "LIVE_TEST: TESTE LIVE 38\n";
  s += "LED_GPIO: 26\nLED_STATE: " + String(remoteLedOn ? "ON" : "OFF") + "\n";
  s += "IP: " + WiFi.localIP().toString() + "\n";
  s += "LOCAL_NAME: " BOTIZIN_LOCAL_NAME ".local\n";
  s += "MDNS: " + String(localNameReady ? "READY" : "WAITING") + "\n";
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
  s += "MODULE_DELIVERY: " + manualOtaStatus + "\n";
  s += "OTA_AUTOMATIC_ENABLED: NO\nOTA_MANUAL_STATUS: " + manualOtaStatus + "\n";
  s += "A22_MODULE_PUSH: " + String(modulePushReady?"READY":"OFF") + "\n";
  s += "MODULE_SOURCE: A22_CACHE_PINNED_GITHUB\n";
  s += "CHECK_INTERVAL_SECONDS: 60\n";
  s += "TELEMETRY: A22_LOCAL\n";
  s += "OLED: " + oledStatus + "\n";
  s += "FACE_FRAMES: " + String(faceFrames.load()) + "\n";
  s += "FACE_MAX_GAP_MS: " + String(faceMaxGapMs.load()) + "\n";
  s += "VOICE_SYNC_MODE: " + String(voiceSync.mode(millis())) + "\n";
  s += "VOICE_SYNC_AGE_MS: " + String(voiceSync.age(millis())) + "\n";
  s += "VOICE_SYNC_EVENTS: " + String(voiceSync.accepted.load()) + "\n";
  s += "VOICE_S3_POLL_OK: " + String(voiceS3HttpOK) + "\n";
  s += "VOICE_S3_POLL_FAIL: " + String(voiceS3HttpFail) + "\n";
  s += "VOICE_S3_AGE_MS: " + String(voiceS3LastMs?uint32_t(millis()-voiceS3LastMs):0) + "\n";
  s += "VOICE_LAB_RETURN_BASE_ARMED: " + String(voiceLabNextBootBaseArmed?"YES":"NO") + "\n";
  s += "ENVIRONMENT_INSTALLED: " + (moduleAvailable?moduleId:String("NONE")) + "\n";
  s += "CATALOG_STATUS: " + catalogStatus + "\n";
  s += "OLED_PAGE: " + String(faceActive.load() ? "BOTIZIN" : oledMenu ? "MENU" : navLevel == 2 ? "DIAGNOSTIC_LIST" : oledPage == 7 ? "DIAGNOSTICS" : oledPage == 8 ? "CONNECTION" : oledPage == 9 ? "CONTROLS" : oledPage == 2 ? "OTA_WROOM" : oledPage==10||oledPage==11 ? "ENVIRONMENTS" : "HELP") + "\n";
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

#include "module_bridge.h"
#include "environment_catalog.h"

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
    pauseFace();
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
    const esp_partition_t *running = esp_ota_get_running_partition();
    bool buttonAuthorized = modulePushReady &&
        (int32_t)(modulePushUntil-millis())>0 &&
        expectedBytes==otaTargetBytes && expectedSha==otaTargetSHA &&
        !otaEnvironment.isEmpty();
    // Fixed architecture: BOTIZIN app0 never writes itself, modules only app1.
    bool correctSlots = running && running->subtype==ESP_PARTITION_SUBTYPE_APP_OTA_0 &&
        destination && destination->subtype==ESP_PARTITION_SUBTYPE_APP_OTA_1;
    if (!otaReady || rebootScheduled || !correctSlots || !buttonAuthorized ||
        !validSha || expectedBytes == 0 || expectedBytes > destination->size) {
      failUpload("invalid metadata, hardware, partitions or pending reboot; Update.begin NOT CALLED"); return;
    }
    if(!saveReturnAnchor()) { failUpload("Menu return anchor could not be saved; no flash writes"); return; }
    if(!saveReturnAnchor()){
      failUpload("cannot validate/save BOTIZIN base return anchor"); return;
    }
    // One-time physical authorization; fail closed if transport aborts.
    modulePushReady=false;
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
    if(!saveInstalledEnvironment(otaEnvironment,otaTargetVersion,expectedSha,expectedBytes,destination)){
      esp_err_t restored=esp_ota_set_boot_partition(originalBoot);
      logLine("RESTORE_BASE_AFTER_METADATA_ERROR: "+String(esp_err_to_name(restored)));
      failUpload("module metadata did not persist; no boot");return;
    }
    uploadActive = false;
    uploadOK = true;
    responseCode = 200;
    manualOtaStatus = "MODULE_VERIFIED_READY_TO_BOOT";
    logLine("OTA_WRITE_VERIFIED: YES; module return anchor written");
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

// WROOM 0.0.40 does not fetch firmware or manifests from cloud.
void checkInternetOTA(bool manual){
  (void)manual;
  internetStatus="MOVIDO_PARA_A22";
  manualOtaStatus="A22_CATALOGO_FIXO";
}
void installInternetOTA(const String &, const String &, const String &, size_t){
  internetStatus="MOVIDO_PARA_A22";
}

// Separate diagnostic sender: no Update, NVS write, boot change or Wi-Fi reconfiguration.
// Called sequentially AFTER the existing OTA check has returned and freed its TLS clients.
// Remote telemetry/RPC moved to A22; the local status route remains available.
void sendTelemetry(){telemetryStatus="A22_LOCAL";}


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
  beginRemoteLed();
  beginOled();
  beginNavigation();
  beginFaceTask();
  delay(1500);
  bool journalOK = journal.begin("wroom-journal", false);
  if (journalOK) priorAttempt = journal.getString("last", "");
  else Serial.println("JOURNAL_OPEN_FAILED");
  loadCatalog();
  loadInstalledModule();
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
  // Lab test safety: when 0.0.41 runs from OTA1, make the intact
  // 0.0.40 base in OTA0 the NEXT boot. This never writes either app image.
  // A normal power-cycle/reset after the pilot therefore returns to app0.
  if(otaReady && run && run->subtype==ESP_PARTITION_SUBTYPE_APP_OTA_1 && a){
    voiceLabNextBootBaseArmed=(esp_ota_set_boot_partition(a)==ESP_OK);
    Serial.println(voiceLabNextBootBaseArmed?"VOICE_LAB_RETURN_BASE_ARMED":"VOICE_LAB_RETURN_BASE_FAILED");
  }
  initializeDiagnosticFlash();
  sampleDiagnostics();
  Serial.println(snapshot());
  loadProvisioning();
  otaAutomaticEnabled=false;
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  if (provisioned) WiFi.begin(wroomSSID.c_str(), wroomPassword.c_str());
  uint32_t start = millis();
  while (provisioned && WiFi.status() != WL_CONNECTED && millis() - start < 30000) delay(100);
  Serial.println(snapshot());
  server.on("/", HTTP_GET, []() { server.send(200, "text/plain; charset=utf-8", snapshot()); });
  server.on("/status", HTTP_GET, []() { server.send(200, "text/plain; charset=utf-8", snapshot()); });
  // LAN-only momentary LED control. Never expose port 80 to the Internet.
  server.on("/led", HTTP_POST, []() {
    IPAddress peer=server.client().remoteIP(), local=WiFi.localIP(), mask=WiFi.subnetMask();
    bool sameSubnet=true;
    for(int i=0;i<4;i++) if((peer[i]&mask[i])!=(local[i]&mask[i])) sameSubnet=false;
    if(!sameSubnet || WiFi.status()!=WL_CONNECTED) {
      server.send(403,"text/plain","LAN only"); return;
    }
    String enabled=server.arg("enabled");
    if(enabled!="1" && enabled!="0") {
      server.send(400,"text/plain","Use POST /led?enabled=1 or enabled=0"); return;
    }
    setRemoteLed(enabled=="1");
    server.send(200,"text/plain",remoteLedOn?"LED_STATE: ON; AUTO_OFF_SECONDS: 30":"LED_STATE: OFF");
  });
  // XiaoZhi voice face notifications may originate ONLY from the known local
  // A22 relay. Presentation-only: no audio, shell, LED, OTA or flash mutation.
  server.on("/face/voice",HTTP_POST,[](){
    const IPAddress peer=server.client().remoteIP();
    const IPAddress a22(192,168,0,14);
    if(WiFi.status()!=WL_CONNECTED || peer!=a22){
      server.send(403,"text/plain","A22 relay required");return;
    }
    if(uploadActive || uploadOK || rebootScheduled){
      server.send(409,"text/plain","WROOM busy");return;
    }
    const String token=server.arg("state");
    if(token!="3" && token!="4" && token!="5" && token!="6"){
      server.send(400,"text/plain","state must be 3,4,5 or 6");return;
    }
    const uint8_t mode=static_cast<uint8_t>(token[0]-'0');
    if(!voiceSync.receive(mode,millis())){
      server.send(400,"text/plain","invalid mode");return;
    }
    server.send(200,"text/plain","VOICE_SYNC_OK");
  });
  server.on("/module/request", HTTP_GET, [](){
    bool ready=modulePushReady && (int32_t)(modulePushUntil-millis())>0 &&
        !uploadActive && !uploadOK && !rebootScheduled;
    if(!ready){server.send(204,"text/plain","");return;}
    String msg="BOTIZIN_MODULE_REQUEST_V1\n";
    msg+="ID: "+otaEnvironment+"\n";
    msg+="VERSION: "+otaTargetVersion+"\n";
    msg+="SIZE: "+String(otaTargetBytes)+"\n";
    msg+="SHA256: "+otaTargetSHA+"\n";
    msg+="BOOT_ID: "+telemetryBootId+"\n";
    msg+="EXPIRES_IN: "+String((modulePushUntil-millis())/1000)+"\n";
    server.send(200,"text/plain; charset=utf-8",msg);
  });
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
  telemetryBootId = String((unsigned long)esp_random(), HEX) + String((unsigned long)esp_random(), HEX);
  server.begin();
  maintainLocalName();
  Serial.println("HTTP_SERVER: port 80; GET /status; POST /update?size=...&sha256=...");
}

void loop() {
  pollRemoteLed();
  maintainLocalName();
  diagnosticLoopTick();
  if (!uploadActive && !uploadOK && !rebootScheduled) sampleDiagnostics();
  if(moduleStartQueued && !uploadActive && !uploadOK && !rebootScheduled) startInstalledModule();
  pollNavigation();
  pollProvisioning(uploadActive || uploadOK || rebootScheduled);
  server.handleClient();
  // Presentation-only: poll real XiaoZhi state while browser is active.
  // Guard other synchronous tasks/OTA from this bounded network read.
  if(!uploadActive && !uploadOK && !rebootScheduled && !otaCheckQueued &&
     !otaInstallQueued && !catalogRefreshQueued)voicePollS3Status();
  if(catalogRefreshQueued && !uploadActive && !uploadOK && !rebootScheduled && !otaInstallQueued && !otaCheckQueued){catalogRefreshQueued=false;checkCatalog();discardNavigation();nextOledRefresh=0;nextTelemetry=millis();}
  if (rebootScheduled && (int32_t)(millis() - rebootAt) >= 0) { Serial.flush(); ESP.restart(); }
  if (!rebootScheduled && !uploadActive && !uploadOK && otaInstallQueued) {
    otaInstallQueued=false;
    // The user confirmed the selected module on the WROOM's physical buttons.
    modulePushReady=true;
    modulePushUntil=millis()+120000;
    manualOtaStatus="AGUARDANDO_A22";
    showOtaProgress("AGUARDANDO A22");
  }
  if(modulePushReady && (int32_t)(millis()-modulePushUntil)>=0) {
    modulePushReady=false;manualOtaStatus="A22_TEMPO_ESGOTADO";
  }
  if (!rebootScheduled && !uploadActive && !uploadOK && otaCheckQueued) {
    otaCheckQueued = false;
    checkInternetOTA(true);
    if (manualOtaStatus == "CHECKING") manualOtaStatus = internetStatus;
    // Presses made while the synchronous query was running cannot confirm its result.
    discardNavigation(); nextOledRefresh = 0; nextTelemetry = millis();
  }
  // Cloud polling removed. A22 handles external services.

  pollNavigation();
  pollFaceIdle();
  refreshOled();
  delay(2);
}
