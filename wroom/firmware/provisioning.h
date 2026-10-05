#pragma once
// Only USB serial can provision. One NVS value; never print credentials.
String wroomSSID, wroomPassword, telemetryDeviceToken;
bool provisioned = false;
#define BOTIZIN_TB_DEVICE_TOKEN telemetryDeviceToken

bool parseConfiguration(cJSON *r, String &ssid, String &password, String &token) {
  cJSON *s = cJSON_GetObjectItemCaseSensitive(r, "ssid");
  cJSON *p = cJSON_GetObjectItemCaseSensitive(r, "password");
  cJSON *t = cJSON_GetObjectItemCaseSensitive(r, "device_token");
  if (!cJSON_IsString(s) || !cJSON_IsString(p) || !cJSON_IsString(t)) return false;
  ssid = s->valuestring; password = p->valuestring; token = t->valuestring;
  if (ssid.length() < 1 || ssid.length() > 32 ||
      (password.length() && (password.length() < 8 || password.length() > 63)) ||
      token.length() < 16 || token.length() > 128 || token.startsWith("tb_")) return false;
  for (size_t i = 0; i < token.length(); ++i)
    if (!isalnum((unsigned char)token[i]) && token[i] != '_' && token[i] != '-') return false;
  return true;
}
void loadProvisioning() {
  Preferences cfg;
  if (!cfg.begin("wroom-config", true)) { Serial.println("CONFIG_REQUIRED"); return; }
  String stored = cfg.getString("config", ""); cfg.end();
  cJSON *r = cJSON_Parse(stored.c_str());
  provisioned = r && parseConfiguration(r, wroomSSID, wroomPassword, telemetryDeviceToken);
  cJSON_Delete(r);
  Serial.println(provisioned ? "CONFIG_LOADED" : "CONFIG_REQUIRED");
}
void pollProvisioning(bool busy) {
  static char input[512]; static size_t used = 0; static bool overflow = false;
  // Bounded work; a USB client cannot starve the network loop.
  for (unsigned n = 0; n < 64 && Serial.available(); ++n) {
    char c = Serial.read();
    if (c == '\r') continue;
    if (c != '\n') {
      if (used < sizeof(input) - 1 && !overflow) input[used++] = c;
      else overflow = true;
      continue;
    }
    input[used] = 0;
    if (overflow || busy) { Serial.println("CONFIG_REJECTED"); }
    else {
      cJSON *r = cJSON_Parse(input);
      cJSON *cmd = cJSON_GetObjectItemCaseSensitive(r, "cmd");
      String ssid, password, token;
      bool valid = cJSON_IsString(cmd) && strcmp(cmd->valuestring, "configure") == 0 &&
                   parseConfiguration(r, ssid, password, token);
      if (valid) {
        Preferences cfg;
        bool saved = cfg.begin("wroom-config", false);
        if (saved) { saved = cfg.putString("config", input) == used; cfg.end(); }
        Serial.println(saved ? "CONFIG_SAVED_REBOOT" : "CONFIG_SAVE_FAILED");
        cJSON_Delete(r);
        memset(input, 0, sizeof(input));
        if (saved) { Serial.flush(); delay(100); ESP.restart(); }
      } else { cJSON_Delete(r); Serial.println("CONFIG_REJECTED"); }
    }
    memset(input, 0, sizeof(input)); used = 0; overflow = false;
  }
}
