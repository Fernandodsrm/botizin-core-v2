#pragma once
#ifndef BOTIZIN_TRIAL_HOST_TEST
#include <esp_timer.h>
#endif

// This image is temporary. Do not change partition layout or write another OTA.
static constexpr size_t trialBaselineBytes = 1166784;
static constexpr const char *trialBaselineSHA = "db682f24db7af0624f4ed0603738d6536643ce7cb5e34eb08a5e99ff4fc8adab";
static const char *trialStatus = "NOT_ARMED";
static bool trialArmed = false;
static esp_timer_handle_t trialTimer = nullptr;

static void trialReturn(void *) { esp_restart(); }

static bool armTimedTrial() {
  const esp_partition_t *run = esp_ota_get_running_partition();
  const esp_partition_t *old = esp_ota_get_next_update_partition(nullptr);
  if (!run || !old || run->address == old->address ||
      old->size != 0x300000 || run->size != 0x300000 ||
      !((run->address == 0x10000 && old->address == 0x310000) ||
        (run->address == 0x310000 && old->address == 0x10000))) {
    trialStatus = "FAILED_PARTITIONS"; return false;
  }
  String actual;
  if (!hashPartition(old, trialBaselineBytes, actual) || actual != trialBaselineSHA) {
    trialStatus = "FAILED_BASELINE_HASH"; return false;
  }
  esp_timer_create_args_t args = {};
  args.callback = trialReturn;
  args.dispatch_method = ESP_TIMER_TASK;
  args.name = "trial_return";
  if (esp_timer_create(&args, &trialTimer) != ESP_OK) {
    trialStatus = "FAILED_TIMER_CREATE"; return false;
  }
  if (esp_timer_start_once(trialTimer, 120000000ULL) != ESP_OK) {
    esp_timer_delete(trialTimer); trialTimer = nullptr;
    trialStatus = "FAILED_TIMER_START"; return false;
  }
  // Arm the next boot before networking. Timer uses the ESP timer task,
  // rather than loop(), so an HTTP wait in loop() cannot postpone it.
  if (esp_ota_set_boot_partition(old) != ESP_OK) {
    esp_timer_stop(trialTimer); esp_timer_delete(trialTimer); trialTimer = nullptr;
    trialStatus = "FAILED_BOOT_SELECTION"; return false;
  }
  const esp_partition_t *boot = esp_ota_get_boot_partition();
  if (!boot || boot->address != old->address) {
    esp_timer_stop(trialTimer); esp_timer_delete(trialTimer); trialTimer = nullptr;
    trialStatus = "FAILED_BOOT_READBACK"; return false;
  }
  trialArmed = true; trialStatus = "ARMED_120_SECONDS_RETURN_0.0.13";
  return true;
}
