#pragma once
// OTA-04: verify device-local health before accepting a new boot image.
// No public MQTT or external service is required to accept firmware.
// No changes to bootloader or OTA partition layout.
#include <stdint.h>

enum class OtaHealthDecision : uint8_t { WAIT, CONFIRM, ROLLBACK, NOT_PENDING };

static constexpr uint32_t OTA02_TIMEOUT_MS = 180000U;
static constexpr uint32_t OTA02_MIN_STABLE_MS = 15000U;

inline OtaHealthDecision otaHealthDecision(bool pendingVerify,
                                           bool partitionsReady,
                                           bool wifiConnected,
                                           bool httpStarted,
                                           uint32_t elapsedMs,
                                           uint32_t healthyDurationMs) {
    if (!pendingVerify) return OtaHealthDecision::NOT_PENDING;
    if (!partitionsReady) return OtaHealthDecision::ROLLBACK;
    if (elapsedMs >= OTA02_TIMEOUT_MS) return OtaHealthDecision::ROLLBACK;
    if (wifiConnected && httpStarted &&
        healthyDurationMs >= OTA02_MIN_STABLE_MS) return OtaHealthDecision::CONFIRM;
    return OtaHealthDecision::WAIT;
}
