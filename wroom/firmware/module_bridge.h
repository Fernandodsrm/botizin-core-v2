#pragma once
// Menu owns installation. A module may only boot the intact recorded Menu.
static String journalField(const String &body,const char *key) {
  String prefix=String(key)+": "; int at=body.indexOf(prefix);
  if(at<0 || (at>0 && body[at-1]!='\n')) return String();
  at+=prefix.length(); int end=body.indexOf('\n',at);
  return body.substring(at,end<0?body.length():end);
}
static bool saveReturnAnchor() {
  const esp_partition_t *run=esp_ota_get_running_partition(); unsigned char digest[32];
  if(!run || esp_partition_get_sha256(run,digest)!=ESP_OK) return false;
  String record=String(run->address)+"|"+hexDigest(digest)+"|"+BOTIZIN_VERSION;
  Preferences prefs; if(!prefs.begin("module-return",false)) return false;
  prefs.putString("menu",record); bool ok=prefs.getString("menu","")==record; prefs.end(); return ok;
}
static void loadInstalledModule() {
  moduleAvailable=false; modulePartition=esp_ota_get_next_update_partition(nullptr);
  if(!modulePartition)return;
  Preferences p;
  if(p.begin("environment",true)){
    moduleId=p.getString("id","");moduleVersion=p.getString("version","");moduleSHA=p.getString("sha","");moduleBytes=p.getUInt("bytes",0);
    if(p.getUInt("address",0)!=modulePartition->address)moduleBytes=0;
    p.end();
  }
  if(moduleId.isEmpty() && journalField(priorAttempt,"TARGET_VERSION")=="0.0.28"){
    moduleId="ps4";moduleVersion="0.0.28";moduleBytes=journalField(priorAttempt,"EXPECTED_BYTES").toInt();
    moduleSHA=journalField(priorAttempt,"SHA_CALCULATED_FLASH");
  }
  String actual;
  if(moduleBytes && moduleBytes<=modulePartition->size && moduleSHA.length()==64 &&
     !moduleId.isEmpty() &&
     hashPartition(modulePartition,moduleBytes,actual) && actual==moduleSHA) moduleAvailable=true;
}
static bool queueModuleStart(const String &boot,const String &sha) {
  if(!moduleAvailable || boot!=telemetryBootId || sha!=moduleSHA ||
     uploadActive || uploadOK || rebootScheduled || otaInstallQueued || otaCheckQueued || moduleStartQueued) return false;
  moduleStartQueued=true; return true;
}
static void startInstalledModule() {
  moduleStartQueued=false; String actual;
  if(!moduleAvailable || !hashPartition(modulePartition,moduleBytes,actual) || actual!=moduleSHA ||
     !saveReturnAnchor() || esp_ota_set_boot_partition(modulePartition)!=ESP_OK) {
    moduleAvailable=false; manualOtaStatus="MODULE_START_REJECTED"; return;
  }
  const esp_partition_t *boot=esp_ota_get_boot_partition();
  if(!boot || boot->address!=modulePartition->address) {manualOtaStatus="MODULE_BOOT_READBACK_FAILED";return;}
  showOtaProgress("ABRINDO AMBIENTE");rebootScheduled=true;rebootAt=millis()+750;
}
