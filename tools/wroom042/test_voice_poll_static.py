#!/usr/bin/env python3
"""Guardrails for direct S3 voice-state lab: no Android/network service required."""
from pathlib import Path
p=Path("wroom/firmware/voice_poll_s3.h").read_text()
fw=Path("wroom/firmware/firmware.ino").read_text()
assert 'http://192.168.0.24/status' in p
assert 'http.GET()' in p
assert 'XIAOZHI_REAL' in p
assert 'web_audio_connected' in p
assert 'cJSON_IsBool' in p and 'cJSON_Delete(root)' in p
assert 'voiceS3PollPeriodMs=650' in p
assert 'http.setConnectTimeout(220)' in p and 'http.setTimeout(200)' in p
assert 'if(mode)voiceSync.receive(mode,millis());' in p
assert '!uploadActive && !uploadOK && !rebootScheduled && !otaCheckQueued' in fw
assert 'voicePollS3Status();' in fw
assert 'VOICE_S3_POLL_OK:' in fw and 'VOICE_S3_POLL_FAIL:' in fw
assert 'esp_ota_set_boot_partition(a)' in fw
print("VOICE_DIRECT_S3_STATIC_GUARDS_OK")
