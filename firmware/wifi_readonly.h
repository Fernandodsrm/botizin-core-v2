#pragma once
// Read-only, explicitly requested Wi-Fi summary for the local panel.
// No credentials, network scanning, active probes, NVS writes or periodic work.
// Available only from the connected STA interface; not from the recovery AP.
static void wifiReadOnlyRegisterRoutes() {
  server.on("/read/wifi", HTTP_GET, []() {
    if (WiFi.status() != WL_CONNECTED ||
        server.client().localIP() != WiFi.localIP()) {
      server.sendHeader("Cache-Control", "no-store");
      server.send(403, "text/plain; charset=utf-8", "LOCAL_STA_ONLY");
      return;
    }
    cJSON *o = cJSON_CreateObject();
    if (!o) {
      server.send(503, "text/plain", "NO_MEMORY");
      return;
    }
    String ssid = WiFi.SSID();
    String ip = WiFi.localIP().toString();
    String gateway = WiFi.gatewayIP().toString();
    String dns = WiFi.dnsIP().toString();
    bool ok = cJSON_AddStringToObject(o, "ssid", ssid.c_str()) &&
              cJSON_AddStringToObject(o, "ip", ip.c_str()) &&
              cJSON_AddStringToObject(o, "gateway", gateway.c_str()) &&
              cJSON_AddStringToObject(o, "dns", dns.c_str()) &&
              cJSON_AddNumberToObject(o, "rssi_dbm", WiFi.RSSI()) &&
              cJSON_AddNumberToObject(o, "channel", WiFi.channel()) &&
              cJSON_AddBoolToObject(o, "mdns_ready", localNameReady) &&
              cJSON_AddBoolToObject(o, "recovery_ap_active", wifiPortalOn);
    char *payload = ok ? cJSON_PrintUnformatted(o) : nullptr;
    cJSON_Delete(o);
    if (!payload) {
      server.send(503, "text/plain", "NO_MEMORY");
      return;
    }
    server.sendHeader("Cache-Control", "no-store");
    server.sendHeader("X-Content-Type-Options", "nosniff");
    server.send(200, "application/json; charset=utf-8", payload);
    cJSON_free(payload);
  });
}
