#pragma once
// BOTIZIN BASE 040: Wi-Fi provisioning exclusively through USB.
// Old configuration blobs may contain legacy fields; these are ignored.
String wroomSSID, wroomPassword;
bool provisioned = false;

static bool parseConfiguration(cJSON *r, String &ssid, String &password) {
  cJSON *s = cJSON_GetObjectItemCaseSensitive(r, "ssid");
  cJSON *p = cJSON_GetObjectItemCaseSensitive(r, "password");
  if (!cJSON_IsString(s) || !cJSON_IsString(p)) return false;
  ssid=s->valuestring;password=p->valuestring;
  return ssid.length()>=1 && ssid.length()<=32 &&
    (password.isEmpty() || (password.length()>=8 && password.length()<=63));
}
void loadProvisioning() {
  Preferences cfg;
  if (!cfg.begin("wroom-config", true)) { Serial.println("WIFI_CONFIG_REQUIRED"); return; }
  String stored=cfg.getString("config","");cfg.end();
  cJSON *r=cJSON_Parse(stored.c_str());
  provisioned=r && parseConfiguration(r,wroomSSID,wroomPassword);
  cJSON_Delete(r);
  Serial.println(provisioned?"WIFI_CONFIG_LOADED":"WIFI_CONFIG_REQUIRED");
}
void pollProvisioning(bool busy) {
  static char input[512];static size_t used=0;static bool overflow=false;
  for(unsigned n=0;n<64&&Serial.available();++n) {
    char c=Serial.read();
    if(c=='\r')continue;
    if(c!='\n') {
      if(used<sizeof(input)-1&&!overflow)input[used++]=c;
      else overflow=true;
      continue;
    }
    input[used]=0;
    if(overflow||busy)Serial.println("WIFI_CONFIG_REJECTED");
    else {
      cJSON *r=cJSON_Parse(input);
      cJSON *cmd=cJSON_GetObjectItemCaseSensitive(r,"cmd");
      String ssid,password;
      bool valid=cJSON_IsString(cmd)&&strcmp(cmd->valuestring,"configure")==0 &&
        parseConfiguration(r,ssid,password);
      if(valid){
        // Do not persist unknown/legacy fields; use only SSID/password.
        cJSON *clean=cJSON_CreateObject();
        cJSON_AddStringToObject(clean,"ssid",ssid.c_str());
        cJSON_AddStringToObject(clean,"password",password.c_str());
        char *canonical=cJSON_PrintUnformatted(clean);
        cJSON_Delete(clean);
        Preferences cfg;bool saved=canonical&&cfg.begin("wroom-config",false);
        if(saved){size_t length=strlen(canonical);saved=cfg.putString("config",canonical)==length;cfg.end();}
        if(canonical)cJSON_free(canonical);
        Serial.println(saved?"WIFI_CONFIG_SAVED_REBOOT":"WIFI_CONFIG_SAVE_FAILED");
        cJSON_Delete(r);memset(input,0,sizeof(input));
        if(saved){Serial.flush();delay(100);ESP.restart();}
      }else{cJSON_Delete(r);Serial.println("WIFI_CONFIG_REJECTED");}
    }
    memset(input,0,sizeof(input));used=0;overflow=false;
  }
}
