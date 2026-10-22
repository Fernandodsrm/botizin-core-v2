#pragma once
// OTA-02 LAB candidate ONLY. Pure deterministic health policy for native unit tests.
// A candidate must show actual outbound MQTT traffic before it is confirmed.
// Broker outages can therefore trigger rollback: DO NOT publish to production.
#include <stdint.h>

enum class OtaHealthDecision : uint8_t { WAIT, CONFIRM, ROLLBACK, NOT_PENDING };

static constexpr uint32_t OTA02_TIMEOUT_MS = 180000U;
static constexpr uint32_t OTA02_MIN_STABLE_MS = 15000U;

inline OtaHealthDecision otaHealthDecision(bool pendingVerify,
                                           bool partitionsReady,
                                           bool wifiConnected,
                                           bool clockSynced,
                                           bool httpStarted,
                                           bool mqttPublishSucceeded,
                                           uint32_t elapsedMs,
                                           uint32_t healthyDurationMs) {
    if (!pendingVerify) return OtaHealthDecision::NOT_PENDING;
    if (!partitionsReady) return OtaHealthDecision::ROLLBACK;
    if (elapsedMs >= OTA02_TIMEOUT_MS) return OtaHealthDecision::ROLLBACK;
    if (wifiConnected && clockSynced && httpStarted && mqttPublishSucceeded &&
        healthyDurationMs >= OTA02_MIN_STABLE_MS) {
        return OtaHealthDecision::CONFIRM;
    }
    return OtaHealthDecision::WAIT;
}
