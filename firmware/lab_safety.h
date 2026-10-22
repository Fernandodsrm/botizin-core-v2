#pragma once
// EXCLUSIVE EXPERIMENTAL BRANCH. Do NOT include in production CORE 0.0.25.
// Experimental ota_1 image NEVER writes ota_0. Returning reselects a verified
// existing CORE 0.0.25 image; it does not download or rewrite any firmware.
static constexpr size_t kCore025Size=1259872;
static constexpr char kCore025SHA[]="bc68a1c45c43f4b0e63d40767521b50290a664725568cd0e6acfae24c37e049a";
static bool labCore025Verified(String &reason) {
  const auto *run=esp_ota_get_running_partition();
  const auto *core=esp_partition_find_first(ESP_PARTITION_TYPE_APP,ESP_PARTITION_SUBTYPE_APP_OTA_0,nullptr);
  if(!run || run->subtype!=ESP_PARTITION_SUBTYPE_APP_OTA_1 ||
     !core || core->address!=0x10000 || core->size!=0x300000) {
    reason="UNEXPECTED_SLOT_LAYOUT";return false;
  }
  esp_ota_img_states_t state;
  if(esp_ota_get_state_partition(core,&state)!=ESP_OK || state!=ESP_OTA_IMG_VALID) {
    reason="CORE_025_NOT_VALID";return false;
  }
  String sha;
  if(!hashPartition(core,kCore025Size,sha) || sha!=kCore025SHA) {
    reason="CORE_025_FLASH_SHA_MISMATCH";return false;
  }
  esp_app_desc_t desc;
  if(esp_ota_get_partition_description(core,&desc)!=ESP_OK) {
    reason="CORE_025_APP_DESC_INVALID";return false;
  }
  reason="CORE_025_VERIFIED";return true;
}
static void labReturnToCore() {
  if(!controlRequireAuth())return;
  if(rebootScheduled || uploadActive || uploadOK || peerOtaBusy) {
    controlReply(409,"LAB_BUSY");return;
  }
  if(server.arg("confirm")!="RETURN_TO_025") {
    controlReply(400,"EXPLICIT_CONFIRMATION_REQUIRED");return;
  }
  String reason;
  if(!labCore025Verified(reason)){controlReply(409,reason);return;}
  const auto *core=esp_partition_find_first(ESP_PARTITION_TYPE_APP,ESP_PARTITION_SUBTYPE_APP_OTA_0,nullptr);
  const esp_err_t result=esp_ota_set_boot_partition(core);
  if(result!=ESP_OK) {
    controlReply(500,String("CORE_BOOT_SELECT_FAILED: ")+esp_err_to_name(result));return;
  }
  const auto *boot=esp_ota_get_boot_partition();
  if(!boot || boot->address!=core->address) {
    controlReply(500,"CORE_BOOT_READBACK_FAILED");return;
  }
  controlReply(202,"CORE_025_VERIFIED. Returning to ota_0 in 2 seconds.");
  rebootAt=millis()+2200UL;
  rebootScheduled=true;
}
static void labRegisterRoutes() {
  server.on("/control/return-core",HTTP_POST,labReturnToCore);
  server.on("/botizin",HTTP_GET,[](){
    server.sendHeader("Cache-Control","no-store");
    server.sendHeader("X-Content-Type-Options","nosniff");
    server.send_P(200,"text/html; charset=utf-8",kBotizinLabHTML);
  });
}
