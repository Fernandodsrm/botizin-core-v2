#!/usr/bin/env python3
"""Static invariants for read-only Wi-Fi and resources panel pilot; no device access."""
from html.parser import HTMLParser
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
fw = (ROOT / "firmware/firmware.ino").read_text()
page = (ROOT / "firmware/panel_page.h").read_text()
route = (ROOT / "firmware/wifi_readonly.h").read_text()
controls = (ROOT / "firmware/control_routes.h").read_text()

assert '#include "wifi_manager.h"\n#include "wifi_readonly.h"' in fw
assert "wifiReadOnlyRegisterRoutes();" in fw
policy = (ROOT / "firmware/lab_policy.h").read_text()
peer = (ROOT / "firmware/peer_ota_server.h").read_text()
assert "#define BOTIZIN_LAB_OTA_DISABLED 1" in policy
assert '#include "lab_policy.h"' in fw
for label in (
    "bool queueOtaCheck()",
    "void beginUpload(const String &length, const String &sha)",
    "void handleUpload()",
    "void checkInternetOTA(bool manual)",
    "void installInternetOTA(const String &targetVersion, const String &targetURL, const String &targetSHA, size_t targetBytes)",
):
    assert label + " {\n#if BOTIZIN_LAB_OTA_DISABLED" in fw, label
assert '#if !BOTIZIN_LAB_OTA_DISABLED\n  if (!rebootScheduled && !uploadActive && !uploadOK && otaInstallQueued)' in fw
assert 'server.on("/update", HTTP_POST, []() {\n    server.send(403, "text/plain; charset=utf-8", "LAB_OTA_DISABLED");' in fw
assert 'OTA_AUTOMATIC_ENABLED: NO (LAB)' in fw
assert 'OTA_LAB_POLICY: FLASH_WRITES_DISABLED' in fw
assert 'cJSON_AddBoolToObject(root, "ota_automatic_enabled", false)' in fw
assert 'cJSON_AddBoolToObject(o,"automatic_enabled",false)' in peer
assert 'if (action != "status")' in peer and 'LAB_OTA_DISABLED' in peer
for fn in ("controlCheckOTA", "controlInstallOTA"):
    assert ("static void " + fn + "() {\n#if BOTIZIN_LAB_OTA_DISABLED") in controls
assert 'LAB_OTA_DISABLED' in controls
assert 'LAB_BUILD: WIFI_RECURSOS_01 (EXPERIMENTAL)' in fw
assert 'server.on("/read/wifi", HTTP_GET' in route
assert "WiFi.status() != WL_CONNECTED" in route
assert "server.client().localIP() != WiFi.localIP()" in route
assert 'LOCAL_STA_ONLY' in route
assert 'server.sendHeader("Cache-Control", "no-store")' in route
assert "wifiPortalOn" in route
assert "WiFi.RSSI()" in route and "WiFi.gatewayIP()" in route and "WiFi.dnsIP()" in route
assert not any(s in route for s in ["WiFi.scanNetworks", "WiFi.begin(", "WiFi.disconnect", "putString(", "ESP.restart()", "HTTP_POST", 'password', 'wifiPortalPassword', 'wifiManagerSavedPassword'])
assert 'server.on("/control/wifi-save",HTTP_POST,controlSaveWiFi)' in controls
assert 'server.on("/control/install-ota",HTTP_POST,controlInstallOTA)' in controls

m = re.search(r'R"BTZN\((.*?)\)BTZN"', page, re.DOTALL)
assert m, "C++ raw HTML literal not found"
html = m.group(1)
class Check(HTMLParser):
    def __init__(self):
        super().__init__()
        self.ids = set()
        self.duplicate_ids = set()
    def handle_starttag(self, tag, attrs):
        for key, value in attrs:
            if key == "id":
                if value in self.ids: self.duplicate_ids.add(value)
                self.ids.add(value)
c = Check()
c.feed(html)
assert not c.duplicate_ids, c.duplicate_ids
required = {
    "page-wifi", "tab-wifi", "wifi-read", "wifi-settings", "wf-ssid",
    "wf-ip", "wf-rssi", "wf-gateway", "wf-dns", "wf-mdns",
    "wf-ap", "wf-feedback", "page-resources", "tab-resources",
    "resources-read", "rs-free", "rs-min", "rs-largest", "rs-psram",
    "rs-ota", "rs-firmware", "rs-loop", "rs-sample-age", "rs-feedback"
}
assert required <= c.ids, required - c.ids
assert "async function loadWifiReadOnly()" in html
assert "async function loadResourceReadOnly()" in html
assert "LAB 01 · EXPERIMENTAL · SEM OTA" in html
assert 'id="controlCheckOTA" type="button" disabled' in html
assert 'id="controlOTAResult" type="button" disabled' in html
assert "await read('/read/wifi')" in html
assert "await read('/status')" in html
assert "textContent" in html and "innerHTML" not in html
assert "loadStatus();loadEvents();" in html
assert "wifi-settings" in html and "$('tab-settings').click()" in html
assert "controlSession" in html and "runControl('/control/wifi-save'" in html
for forbidden in ("setInterval(", "WebSocket(", "EventSource(", "location.reload(", "eval("):
    assert forbidden not in html, forbidden
assert "firmware/firmware.ino" not in html
print("S3_WIFI_RECURSOS_READONLY_STATIC_OK", len(html), "HTML bytes")
