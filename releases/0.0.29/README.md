# BOTIZIN XiaoZhi LAB 0.0.29

- Source: private `Fernandodsrm/botizin-core`, commit `b526865f0467d1023d5c89bd27b5a7f472988962`.
- GitHub Actions successful: https://github.com/Fernandodsrm/botizin-core/actions/runs/39914092076
- Image: ESP32-S3, 1946688 bytes, SHA-256 `ac4e7faefd5ead0bfb62acaf60ebf7919ac345d213fc794154f85de5f92d90e0`.
- Fix: split decoded 60 ms / 24 kHz PCM speech output into 480-sample chunks; expose speaker queued/dropped counters.
- Safety: installation only from CORE 0.0.25 in ota_0 with ota_1 target; lab auto-reverts if not owner-confirmed within 20 minutes; leave production `manifest.json` on CORE 0.0.25.
- Real voice response is not yet validated on physical device. Confirm LAB only after hearing audio.

Run `tools/install-xiaozhi-029-ram-ota.ps1` on Windows PowerShell connected to local S3 once the CORE 0.0.25 is active.
