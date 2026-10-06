#include "../firmware/ota_confirmation.h"
#include <cassert>
#include <cstdio>
int main() {
  auto match = [](uint32_t now, uint32_t until, const char *id="candidate", const char *v="0.0.7", const char *sha="digest", const char *boot="boot") {
    return otaConfirmationMatches(now, until, id, "candidate", v, "0.0.7", sha, "digest", boot, "boot");
  };
  assert(match(100, 200));
  assert(!match(200, 200)); assert(!match(201, 200)); assert(!match(100, 0));
  assert(!match(100, 200, "old")); assert(!match(100, 200, "candidate", "0.0.6"));
  assert(!match(100, 200, "candidate", "0.0.7", "changed"));
  assert(!match(100, 200, "candidate", "0.0.7", "digest", "previous-boot"));
  assert(!match(100, 200, nullptr));
  assert(match(0xfffffff0U, 0x100U)); assert(!match(0x101U, 0x100U));
  puts("OTA_CONFIRMATION_TESTS_OK: candidate/version/hash/boot binding, expiry, wraparound");
}
