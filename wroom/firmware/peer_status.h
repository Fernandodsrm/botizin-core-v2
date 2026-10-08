#pragma once
#include "peer_status_parser.h"

// Read-only link to the S3's existing local status server. No S3 firmware change.

static PeerStatusFrame peerFrame = {};
static bool peerHaveReport = false;
static uint32_t nextPeerPoll = 0;
static char peerBody[8193];

static bool peerRecent() {
  return peerHaveReport && WiFi.status() == WL_CONNECTED && millis() - peerSeenAt < 30000;
}

static void pollPeerStatus() {
  if (uploadActive || uploadOK || rebootScheduled || ESP.getFreeHeap() < 85000) return;
  if (WiFi.status() != WL_CONNECTED) { peerStatus = "WIFI_DISCONNECTED"; return; }
  if ((int32_t)(millis() - nextPeerPoll) < 0) return;
  nextPeerPoll = millis() + (!oledMenu && navBoard && (oledPage == 7 || oledPage == 8) ? 10000 : 30000);
  DiagnosticScope diagnosticScope(DIAG_PEER);diagnosticScope.failed=true;
  String base=peerBaseURL();
  if(base.isEmpty()) {peerStatus="ADDRESS_NOT_FOUND";return;}
  NetworkClient client;
  HTTPClient http;
  http.setConnectTimeout(350); http.setTimeout(400);
  http.useHTTP10(true); http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
  peerStatus = "CONNECT_FAILED";
  if (http.begin(client, base+"/status")) {
    int code = http.GET(), length = http.getSize();
    peerStatus = "HTTP_" + String(code);
    if (code == 200 && length > 0 && length <= 8192) {
      NetworkClient *stream = http.getStreamPtr();
      size_t received = 0; uint32_t started = millis();
      while (received < (size_t)length && millis() - started < 750) {
        int available = stream->available();
        if (available > 0) {
          size_t wanted = (size_t)available;
          if (wanted > (size_t)length - received) wanted = length - received;
          int count = stream->read((uint8_t *)peerBody + received, wanted);
          if (count > 0) received += count;
        } else if (!stream->connected()) break;
        else delay(1);
      }
      peerBody[received] = 0;
      PeerStatusFrame candidate = {};
      if (received == (size_t)length && parsePeerStatus(peerBody, candidate)) {
        peerFrame = candidate; peerHaveReport = true; peerSeenAt = millis();
        peerVersion = candidate.version; peerStatus = "OK";diagnosticScope.failed=false;
      } else peerStatus = "INVALID_OR_INCOMPLETE_STATUS";
    } else if (code == 200) peerStatus = "INVALID_LENGTH";
  }
  http.end(); client.stop();
  if(peerStatus!="OK")invalidatePeerAddress();
  Serial.println("S3_LINK: " + peerStatus + (peerHaveReport ? " / last_version=" + peerVersion : ""));
}
