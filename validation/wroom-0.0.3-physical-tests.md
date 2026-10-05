# WROOM 0.0.3 validated baseline

Physical evidence supplied by Fernando on 2026-10-05, America/Sao_Paulo.

## Build and recovery

ESP32-D0WD-V3, 4MB flash, no PSRAM, CH9102 COM6, MAC 30:76:F5:90:67:E0. Existing six-partition layout retained, app0 0x10000 and app1 0x150000, each 1310720 bytes.

Original full V5 SESSION CORE 0.3 / Bluepad32 recovery backup: 4194304 bytes, SHA256 670ebe0e155e1e0cfe8169e55397e9e792e40bb23d846773d941a96643289e37. Stored on user's Windows notebook.

Cloud source commit 02886d87c860e6c1f62ae76d202d4a170ece933c; Arduino CLI 1.5.1, esp32 core 3.3.12. ci/build_wroom.py generates versions 0.0.1, 0.0.2, 0.0.3 from the same source, overriding version.h in temporary build folders. Cloud run 37270596752 passed all image-chip, size and partition checks. 0.0.3 binary: 1170400 bytes, SHA256 a7b4b0a991d5bb4ee0efd69659b3a72385c53e751e286cda8cfbb302a4ad1d77.

Source preservation ref: wroom-source-0.0.3-intocavel. Published release preservation ref: wroom-release-0.0.3-intocavel at 19d7237968eac673fc9d707020e4c33f8e94465f. These are preservation branches, not an assertion of enforced branch protection.

## Physical tests confirmed

1. USB 0.0.1: configuration saved privately, CONFIG_LOADED, WIFI OK, IP 192.168.0.38, telemetry HTTP_200.
2. Automatic HTTPS OTA 0.0.1 → 0.0.2: expected, received and flash hashes agree; Update.end true, error 0; app0 → app1; 0.0.2 VALID, Wi-Fi and telemetry restored.
3. Reported power-cycle/restart capture 0.0.2: POWERON, app1 VALID, configuration recovered, Wi-Fi OK, UP_TO_DATE 0.0.2, telemetry HTTP_200.
4. Automatic HTTPS OTA 0.0.2 → 0.0.3: expected, received and flash SHA all a7b4b0a991d5bb4ee0efd69659b3a72385c53e751e286cda8cfbb302a4ad1d77; 1170400 bytes; Update.end true, error 0, image description ESP_OK; app1 → app0. Software boot 0.0.3 VALID, Wi-Fi OK, UP_TO_DATE 0.0.3, telemetry HTTP_200.
5. Final reported USB disconnect/reconnect plus follow-up RESET/EN capture: runtime 0.0.3, POWERON (1), uptime 5 seconds, running and boot app0 at 0x10000, VALID, next app1, Wi-Fi OK at 192.168.0.38, UP_TO_DATE 0.0.3, fresh telemetry HTTP_200.

## Scope and next step

WROOM channel main/wroom/manifest.json and ThingsBoard device BOTIZIN-WROOM 368826d0-c082-11f1-8cda-091689c2cb7f are separate from the S3. No credentials appear here. S3 0.0.10 manifest and firmware remain unchanged.

WROOM diagnostic PING is implemented but not yet physically validated. Next: dedicated private WROOM panel and correlated real PONG; no new firmware is required for that test. OLED, buttons, external LED and Bluepad32 are not included in this minimal network baseline and must be integrated separately without modifying the preserved baseline.
