#!/usr/bin/env python3
"""Read-only static invariants for 0.0.43; full compile is separate CI."""
from pathlib import Path
fw=Path('wroom/firmware/firmware.ino').read_text(encoding='utf-8')
v=Path('wroom/firmware/version.h').read_text(encoding='utf-8')
p=Path('wroom/firmware/voice_poll_s3.h').read_text(encoding='utf-8')
assert '0.0.43' in v
assert 'VOICE_STABLE_BOOT:' in fw
assert 'VOICE_STABLE_RECOVERY_BASE:' in fw
assert 'voiceStablePilot' in fw and 'voiceStableBootConfirmed' in fw
assert 'voiceLabNextBootBaseArmed=(esp_ota_set_boot_partition(a)==ESP_OK)' in fw
assert 'uint32_t(millis()-voiceStableStartMs)>=60000' in fw
assert 'faceDisplayEnabled.load() && faceFrames.load()>=100' in fw
assert 'ESP.getFreeHeap()>90000 && voiceS3HttpOK>=25' in fw
assert 'uint32_t(millis()-voiceS3LastMs)<=2500' in fw
assert 'esp_ota_set_boot_partition(candidate)==ESP_OK' in fw
assert 'voiceLabNextBootBaseArmed=false;' in fw
assert 'http://192.168.0.24/status' in p and 'XIAOZHI_REAL' in p
assert 'http.setConnectTimeout(220)' in p and 'http.setTimeout(200)' in p
assert 'voiceSync.receive(mode,millis())' in p
print('BOTIZIN_WROOM043_STABLE_GUARDS_OK')
