# WROOM 0.0.2 physical OTA and restart validation

User serial captures on 2026-10-05, America/Sao_Paulo.

0.0.1 → 0.0.2 via authenticated HTTPS. 1170400 bytes written; expected, received and flash SHA256 all 5fc9ea1e3a681d21824e9a3528fba56fd27a5d395751786d60b347490e072eb1. Update.end true, error 0, image description ESP_OK. Boot switched app0 → app1. Software reboot started BOTIZIN WROOM V0.0.2 on app1 with VALID OTA state, CONFIG_LOADED, WIFI OK at 192.168.0.38, UP_TO_DATE 0.0.2, TELEMETRY HTTP_200.

After instructed USB disconnect/reconnect and follow-up RESET/EN, user provided a POWERON (1) boot capture: runtime 0.0.2, running and boot app1 at 0x150000, VALID, next app0. CONFIG_LOADED, WIFI OK and fresh TELEMETRY HTTP_200. This confirms persistence across the reported power-cycle/restart test. No WROOM PING response has yet been captured.

Next: publish 0.0.3 and verify app1 → app0 plus fresh telemetry, then final power-cycle. The S3 0.0.10 manifest and firmware remain unchanged.
