#pragma once
// BOTIZIN 0.0.17 - Experimental, publish-only MQTT health snapshots.
// NO commands, OTA changes, partition writes or device secrets.
// broker.emqx.io is a PUBLIC TEST BROKER. Payload deliberately excludes
// device identifiers, IP, SSID, keys, GPIO status and OTA journal.
#include <PubSubClient.h>

static NetworkClientSecure botizinMqttTls;
static PubSubClient botizinMqtt(botizinMqttTls);
static uint32_t botizinMqttNextAt = 0;
static uint32_t botizinMqttAttempts = 0;
static uint32_t botizinMqttSuccesses = 0;
static String botizinMqttLast = "NOT_STARTED";

static constexpr char kBotizinMqttHost[] = "broker.emqx.io";
static constexpr uint16_t kBotizinMqttPort = 8883;
static constexpr char kBotizinMqttTopic[] =
    "fernando-lab/botizin-lab-s3-mqtt-readonly-20261021/state";

// New firmware must also retain normal built-in HTTPS OTA code paths.
static void botizinMqttSetup() {
    botizinMqttNextAt = millis() + 35000; // allow early OTA check and Wi-Fi settling
    botizinMqtt.setServer(kBotizinMqttHost, kBotizinMqttPort);
    botizinMqtt.setKeepAlive(15);
    botizinMqtt.setSocketTimeout(3);
    botizinMqtt.setBufferSize(384);
    botizinMqttTls.useBuiltinCACertBundle();
    botizinMqttTls.setHandshakeTimeout(5);
    botizinMqttTls.setTimeout(5000);
}

static void botizinMqttLoop() {
    if ((int32_t)(millis() - botizinMqttNextAt) < 0) return;
    // No continuous MQTT TCP session and no retries within this cycle.
    // Avoid TLS competing with ongoing firmware downloads and recovery.
    botizinMqttNextAt = millis() + 90000;
    if (uploadActive || uploadOK || rebootScheduled || peerOtaBusy ||
        otaCheckQueued || otaInstallQueued) return;
    if (WiFi.status() != WL_CONNECTED || time(nullptr) < 1700000000) {
        botizinMqttLast = "NO_WIFI_OR_NTP"; return;
    }
    if (ESP.getFreeHeap() < 140000) {
        botizinMqttLast = "LOW_HEAP"; return;
    }

    ++botizinMqttAttempts;
    const esp_partition_t* active = esp_ota_get_running_partition();
    const char* slot = "unknown";
    if (active) {
        if (active->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_0) slot = "ota_0";
        if (active->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_1) slot = "ota_1";
    }
    esp_ota_img_states_t state = ESP_OTA_IMG_UNDEFINED;
    const bool valid = active &&
        esp_ota_get_state_partition(active, &state) == ESP_OK &&
        state == ESP_OTA_IMG_VALID;
    // Payload is intentionally low-sensitivity and informational only.
    // Only an OUTBOUND summary on the existing 90s MQTT schedule.
    // Started != reachable; 'http_seen' proves a previous request was served,
    // http_age_s is the age of the most recent successful request, not a probe.
    char data[340];
    snprintf(data, sizeof(data),
             "{\"firmware\":\"%s\",\"uptime_s\":%lu,\"slot\":\"%s\","
             "\"ota_valid\":%s,\"ota_ready\":%s,\"heap_kb\":%lu,"
             "\"attempt\":%lu,\"http_started\":%s,\"http_seen\":%s,"
             "\"http_age_s\":%ld,\"last_event\":\"%s\",\"role\":\"s3-lab-readonly\"}",
             BOTIZIN_VERSION,
             (unsigned long)(millis()/1000),slot,
             valid?"true":"false",otaReady?"true":"false",
             (unsigned long)(ESP.getFreeHeap()/1024),
             (unsigned long)botizinMqttAttempts,
             eventHttpServerStarted?"true":"false",
             eventHttpConfirmed?"true":"false",
             eventHttpConfirmed?(long)((millis()-eventHttpLastGetMs)/1000):-1L,
             botizinEvents.count?eventName(botizinEvents.oldest(botizinEvents.count-1).code):"NONE");

    // Only anonymous public test telemetry. Do not subscribe to ANY topics.
    // Never accept commands or URLs from this broker.
    char clientId[40];
    snprintf(clientId, sizeof(clientId), "botizin-s3-lab-%08lx",
             (unsigned long)esp_random());
    if (botizinMqtt.connect(clientId)) {
        bool sent = botizinMqtt.publish(kBotizinMqttTopic, data, false);
        botizinMqttLast = sent ? "PUBLISHED" : "PUBLISH_FAILED";
        if (sent) ++botizinMqttSuccesses;
        botizinMqtt.disconnect();
    } else {
        botizinMqttLast = "CONNECT_FAILED";
    }
    botizinMqttTls.stop();
    eventJournalMqttResult(botizinMqttLast=="PUBLISHED");
    Serial.printf("MQTT_READONLY: %s attempts=%lu successes=%lu\n",
                  botizinMqttLast.c_str(),
                  (unsigned long)botizinMqttAttempts,
                  (unsigned long)botizinMqttSuccesses);
}

static String mqttStatusSnapshot() {
    return String("MQTT_LAST: ") + botizinMqttLast + "\n" +
           "MQTT_ATTEMPTS: " + String((unsigned long)botizinMqttAttempts) + "\n" +
           "MQTT_SUCCESSES: " + String((unsigned long)botizinMqttSuccesses) + "\n";
}
