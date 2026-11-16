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

assert '#include "wifi_manager.h"\n#include "wifi_trial.h"\n#include "wifi_readonly.h"' in fw
assert "wifiReadOnlyRegisterRoutes();" in fw
assert "wifiTrialTick();" in fw
assert "wifiTrialStatus();" in fw
trial = (ROOT / "firmware/wifi_trial.h").read_text()
wifi = (ROOT / "firmware/wifi_manager.h").read_text()
assert "WIFI_TRIAL_CONFIRM_MS = 120000UL" in trial
assert "WIFI_TRIAL_CONNECT_MS = 25000UL" in trial
assert "wifiTrialPhase=WIFI_TRIAL_QUEUED" in trial
assert "wifiTrialPhase=WIFI_TRIAL_CONFIRM_REQUIRED" in trial
assert "wifiTrialPhase=WIFI_TRIAL_COMMITTED" in trial
assert "wifiTrialPhase=WIFI_TRIAL_ROLLED_BACK" in trial
assert "wifiManagerStoreCredentials(wifiTrialTargetSsid,wifiTrialTargetPassword)" in trial
assert "if(wifiTrialPhase!=WIFI_TRIAL_CONFIRM_REQUIRED ||" in trial
assert "wifiManagerStoreCredentials(wifiTrialTargetSsid,wifiTrialTargetPassword)" in trial
assert "WiFi.persistent(false)" in trial and "WiFi.setAutoReconnect(false)" in trial
assert "wifiTrialRestore(\"CONFIRM_TIMEOUT\")" in trial
assert "if(wifiTrialActive()) return;" in wifi
assert "wifiManagerStoreCredentials(ssid,pass)" not in controls
for endpoint in ("/control/wifi-test","/control/wifi-confirm","/control/wifi-state","/control/wifi-scan"):
    assert endpoint in controls, endpoint
assert "if(!controlRequireAuth())return;" in controls
assert "controlReply(409,\"WIFI2_USE_TEST_AND_CONFIRM\");" in controls
policy = (ROOT / "firmware/lab_policy.h").read_text()
peer = (ROOT / "firmware/peer_ota_server.h").read_text()
assert "#define BOTIZIN_LAB_OTA_DISABLED 0" in policy
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
assert 'OTA_AUTOMATIC_ENABLED: YES' in fw
assert 'OTA_CANDIDATE_POLICY: OTA_ORIGINAL_RESTORED' in fw
assert 'cJSON_AddBoolToObject(root, "ota_automatic_enabled", true)' in fw
assert 'cJSON_AddBoolToObject(o,"automatic_enabled",true)' in peer
assert 'if (action != "status")' in peer and 'LAB_OTA_DISABLED' in peer
for fn in ("controlCheckOTA", "controlInstallOTA"):
    assert ("static void " + fn + "() {\n#if BOTIZIN_LAB_OTA_DISABLED") in controls
assert 'LAB_OTA_DISABLED' in controls
assert 'CANDIDATE_BUILD: WIFI2_CORE_026 (EXPERIMENTAL)' in fw
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
    "page-wifi", "tab-wifi", "wifi-read", "wf-scan", "wf-ssid",
    "wf-ip", "wf-rssi", "wf-gateway", "wf-dns", "wf-mdns",
    "wf-ap", "wf-feedback", "page-resources", "tab-resources",
    "resources-read", "rs-free", "rs-min", "rs-largest", "rs-psram",
    "rs-ota", "rs-firmware", "rs-loop", "rs-sample-age", "rs-feedback"
}
assert required <= c.ids, required - c.ids
assert "async function loadWifiReadOnly()" in html
assert "async function loadResourceReadOnly()" in html
assert "WIFI 2.0 · CORE 0.0.26 CANDIDATA" in html
assert 'id="controlCheckOTA" type="button"' in html
assert 'id="controlOTAResult" type="button"' in html
assert "await read('/read/wifi')" in html
assert "await read('/status')" in html
assert "textContent" in html and "innerHTML" not in html
assert "loadStatus();loadEvents();" in html
assert "wf-admin-login" in c.ids
assert "wf-change-form" in c.ids and "wf-confirm" in c.ids
assert "wf-scan" in c.ids and "wf-network-choice" in c.ids
assert "settingsGotoWifi" in c.ids
assert "await wifiCommand('/control/wifi-test',{ssid,password})" in html
assert "await wifiCommand('/control/wifi-confirm')" in html
assert "await wifiCommand('/control/wifi-state')" in html
assert "await wifiCommand('/control/wifi-scan')" in html
assert "controlSession" in html and "controlPost('/control/login',{password:pass},false)" in html
assert "runControl('/control/wifi-save'" not in html
for forbidden in ("setInterval(", "WebSocket(", "EventSource(", "location.reload(", "eval("):
    assert forbidden not in html, forbidden
assert "firmware/firmware.ino" not in html
print("S3_WIFI_RECURSOS_READONLY_STATIC_OK", len(html), "HTML bytes")

# Candidate build must restore the original OTA execution on purpose.
assert '#define BOTIZIN_VERSION "0.0.26"' in (ROOT/"firmware/version.h").read_text()
assert '#if !BOTIZIN_LAB_OTA_DISABLED' in fw
assert 'checkInternetOTA(false);' in fw
assert 'installInternetOTA(v,u,h,n)' in fw
assert 'server.on("/update", HTTP_POST' in fw
assert "botizinMqttLoop();" in fw
assert "sendTelemetry();" in fw
assert "wifiTrialTick();" in fw
