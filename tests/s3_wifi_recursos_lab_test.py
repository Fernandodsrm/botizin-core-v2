#!/usr/bin/env python3
"""BOTIZIN Wi-Fi 2.1: validate one-login panel, test-and-return, separate save, OTA and MQTT."""
from html.parser import HTMLParser
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
read = lambda p: (ROOT / p).read_text(encoding="utf-8")
fw = read("firmware/firmware.ino")
page = read("firmware/panel_page.h")
controls = read("firmware/control_routes.h")
wifi = read("firmware/wifi_manager.h")
trial = read("firmware/wifi_trial.h")
route = read("firmware/wifi_readonly.h")
policy = read("firmware/lab_policy.h")

assert '#define BOTIZIN_VERSION "0.0.27"' in read("firmware/version.h")
assert "#define BOTIZIN_LAB_OTA_DISABLED 0" in policy
assert 'CANDIDATE_BUILD: WIFI2_UX_027 (EXPERIMENTAL)' in fw
assert 'OTA_AUTOMATIC_ENABLED: YES' in fw
assert 'OTA_CANDIDATE_POLICY: OTA_ORIGINAL_RESTORED' in fw
assert 'botizinMqttLoop();' in fw
assert 'checkInternetOTA(false);' in fw
assert 'server.on("/update", HTTP_POST' in fw
assert "thingsboard.cloud" not in fw and "BOTIZIN_TB_DEVICE_TOKEN" not in fw
assert "sendTelemetry();" not in fw
assert 'wifiTrialTick();' in fw
assert '#include "wifi_manager.h"\n#include "wifi_trial.h"\n#include "wifi_readonly.h"' in fw

# An explicit test can disconnect the STA temporarily, then MUST return
# to the old SSID before reporting success, without saving credentials.
assert 'WIFI_TRIAL_CONNECT_MS=25000UL' in trial
assert 'WIFI_TRIAL_OBSERVE_MS=3500UL' in trial
assert 'WIFI_TRIAL_RETURN_MS=30000UL' in trial
assert 'WIFI_TRIAL_SAVE_MS=180000UL' in trial
for value in ("WIFI_TRIAL_QUEUED", "WIFI_TRIAL_CONNECTING", "WIFI_TRIAL_OBSERVING",
              "WIFI_TRIAL_RETURNING", "WIFI_TRIAL_TEST_OK", "WIFI_TRIAL_TEST_FAILED",
              "WIFI_TRIAL_RECOVERY_FAILED"):
    assert value in trial, value
assert 'wifiTrialBeginReturn("CONNECT_TIMEOUT")' in trial
assert 'wifiTrialBeginReturn("TARGET_CONNECTED")' in trial
assert 'WiFi.SSID()==wifiTrialOldSsid' in trial
assert 'wifiTrialConnectedToTarget?WIFI_TRIAL_TEST_OK:WIFI_TRIAL_TEST_FAILED' in trial
assert 'WIFI_TRIAL_TARGET_IP' in trial
assert 'WIFI_TRIAL_REASON' in trial
assert 'wifiTrialSaveReady()' in trial and 'wifiTrialSave()' in trial
assert 'wifiManagerStoreCredentials(wifiTrialTargetSsid,wifiTrialTargetPassword)' in trial
assert trial.index('static bool wifiTrialSave()') < trial.index('static void wifiTrialTick()')
assert 'wifiTrialPhase=WIFI_TRIAL_COMMITTED' in trial
assert 'if(wifiTrialActive()) return;' in wifi

# The server sends actual dashboard HTML only after a valid browser cookie.
assert "kBotizinLoginHTML" in wifi
assert "if(!controlSessionValid())" in wifi
assert "server.send_P(200," in wifi
assert '"Cookie"' in controls
assert 'server.header("Cookie")' in controls
assert 'HttpOnly; SameSite=Strict' in controls
assert 'controlSessionUntil=millis()+600000UL' in controls
assert 'server.client().remoteIP()!=controlSessionIP' in controls
assert '"AUTH_OK"' in controls
assert 'if(!controlRequireAuth())return;' in controls
assert 'if(wifiTrialSave())' in controls
for endpoint in ('/control/wifi-scan','/control/wifi-test',
                 '/control/wifi-state','/control/wifi-confirm'):
    assert endpoint in controls, endpoint
assert 'WIFI_TEST_STARTED' in controls
assert 'WIFI_SAVED_FOR_NEXT_RESTART' in controls

# Never expose Wi-Fi credentials through the readonly route.
assert 'server.on("/read/wifi", HTTP_GET' in route
for forbidden in ("WiFi.scanNetworks", "WiFi.begin(", "putString(", "ESP.restart()",
                  "HTTP_POST", "wifiPortalPassword", "wifiManagerSavedPassword"):
    assert forbidden not in route, forbidden

m = re.search(r'R"BTZN\((.*?)\)BTZN"', page, re.DOTALL)
assert m, "C++ dashboard literal missing"
html = m.group(1)

class Check(HTMLParser):
    def __init__(self):
        super().__init__()
        self.ids = set()
        self.duplicates = set()
    def handle_starttag(self, tag, attrs):
        for key, value in attrs:
            if key == "id":
                if value in self.ids: self.duplicates.add(value)
                self.ids.add(value)

c = Check()
c.feed(html)
assert not c.duplicates, c.duplicates
for required in ("page-wifi", "tab-wifi", "wf-ssid", "wf-ip",
                 "wf-rssi", "wf-channel", "wf-gateway", "wf-dns",
                 "wf-mdns", "wf-ap", "wf-scan", "wf-network-choice",
                 "wf-target-ssid", "wf-target-pass", "wf-change-form",
                 "wf-test", "wf-confirm", "wf-test-result"):
    assert required in c.ids, required
assert "wf-stage-state" not in c.ids
assert "wf-admin-login" not in c.ids
assert "wf-admin-pass" not in c.ids
assert "WIFI 2.1 · CORE 0.0.27 CANDIDATA" in html
assert "function renderWifiTrial(d)" in html
assert "function startWifiWatch()" in html
assert "window.setTimeout(tick,2400)" in html
assert "wifiWatchId" in html
assert "wifiCommand('/control/wifi-test',{ssid,password})" in html
assert "wifiCommand('/control/wifi-confirm')" in html
assert "wifiCommand('/control/wifi-state',{},true)" in html
assert "credentials:'same-origin'" in html
assert "innerHTML" not in html
assert "setInterval(" not in html
assert "WebSocket(" not in html
assert "EventSource(" not in html
assert "eval(" not in html
print("WIFI21_STATIC_OK", len(html), "HTML bytes")
