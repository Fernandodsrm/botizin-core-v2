#pragma once
#include <stdint.h>
#include <string.h>

// Wrap-safe expiry and exact binding to the candidate and the current boot.
inline bool otaConfirmationMatches(uint32_t now, uint32_t until,
    const char *id, const char *expectedId, const char *version, const char *expectedVersion,
    const char *sha, const char *expectedSha, const char *boot, const char *expectedBoot) {
  return until && (int32_t)(until - now) > 0 && id && expectedId && *expectedId &&
    version && expectedVersion && sha && expectedSha && boot && expectedBoot &&
    strcmp(id, expectedId) == 0 && strcmp(version, expectedVersion) == 0 &&
    strcmp(sha, expectedSha) == 0 && strcmp(boot, expectedBoot) == 0;
}
