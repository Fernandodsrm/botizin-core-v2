#pragma once
// LAB-only WROOM <= S3 XiaoZhi state reader, no A22 or browser proxy.
// This is presentation data only. Never writes NVS, OTA, GPIO or secrets.
#include <WiFi.h>
#include <HTTPClient.h>
#include <cJSON.h>
#include "voice_sync_state.h"

static uint32_t voiceS3NextPollAt=0;
static uint32_t voiceS3HttpOK=0;
static uint32_t voiceS3HttpFail=0;
static uint32_t voiceS3LastMs=0;
static constexpr uint32_t voiceS3PollPeriodMs=650;
static constexpr char voiceS3StatusUrl[]="http://192.168.0.24/status";

static bool voiceParseS3Status(const char*json, uint8_t &mode) {
  if(!json) return false;
  cJSON *root=cJSON_Parse(json);
  if(!root) return false;
  cJSON *origin=cJSON_GetObjectItemCaseSensitive(root,"origin");
  cJSON *state=cJSON_GetObjectItemCaseSensitive(root,"state");
  cJSON *connected=cJSON_GetObjectItemCaseSensitive(root,"web_audio_connected");
  bool ok=cJSON_IsString(origin) && strcmp(origin->valuestring,"XIAOZHI_REAL")==0 &&
          cJSON_IsNumber(state) && state->valuedouble>=3 && state->valuedouble<=9 &&
          (state->valueint==3 || state->valueint==4 || state->valueint==5 ||
           state->valueint==6 || state->valueint==9) && cJSON_IsBool(connected);
  if(ok) {
    // No active browser mic/speaker: let WROOM return to its normal autonomous face.
    mode=cJSON_IsTrue(connected)?(state->valueint==9?4:(uint8_t)state->valueint):0;
  }
  cJSON_Delete(root);
  return ok;
}

static void voicePollS3Status() {
  const uint32_t now=millis();
  if((int32_t)(now-voiceS3NextPollAt)<0)return;
  voiceS3NextPollAt=now+voiceS3PollPeriodMs;
  if(WiFi.status()!=WL_CONNECTED)return;  // WROOM face uses TTL fallback
  WiFiClient socket;
  socket.setTimeout(200);
  HTTPClient http;
  http.setConnectTimeout(220);
  http.setTimeout(200);
  if(!http.begin(socket,voiceS3StatusUrl)) {
    ++voiceS3HttpFail;
    return;
  }
  // GET only; no headers/tokens or data leaving the home LAN.
  http.addHeader("Connection","close");
  const int status=http.GET();
  if(status==200) {
    // S3 /status returns <640 bytes. Bound parsing and reject unrelated JSON.
    const int n=http.getSize();
    if(n>1200) {
      ++voiceS3HttpFail;
    } else {
      String response=http.getString();
      uint8_t mode=0;
      if(response.length()<=1200 && voiceParseS3Status(response.c_str(),mode)) {
        if(mode)voiceSync.receive(mode,millis());
        // If mode is 0 (no web audio), stop feeding voiceSync; after 4s,
        // the ordinary autonomous face resumes.
        ++voiceS3HttpOK;
        voiceS3LastMs=millis();
      } else ++voiceS3HttpFail;
    }
  } else ++voiceS3HttpFail;
  http.end();
}
