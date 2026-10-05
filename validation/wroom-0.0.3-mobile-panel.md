# WROOM native mobile dashboard validation

Fernando supplied four screenshots on 2026-10-05 at 03:47–03:48 America/Sao_Paulo.

Native ThingsBoard dashboard renders on Android: BOTIZIN WROOM / ESP32-WROOM, CONTACT RECENT, firmware 0.0.3, OTA VALID, last real contact timestamp 03:47:29. The panel shows app0 / ota_0 at 0x10000 for running and boot, size 1310720, reset POWERON (1), uptime at last report 9m25s, boot 3852912ef418cdea, report sequence 10. Last OTA record version 0.0.3, 1170400 bytes, flash SHA matching published image.

User clicked Enviar PING in the native dashboard. Screenshot confirms PONG in 13.1 seconds, command_id ping-1791182893274-0cd952ce, firmware 0.0.3, boot 3852912ef418cdea, running and boot app0, OTA VALID, reset POWERON (1). Same boot as the earlier authenticated API PONG test. This completes browser-button round-trip validation in addition to the cloud API test.

No firmware, manifest, S3 dashboard, GPIO or credentials changed during this validation. Preserved baseline remains WROOM 0.0.3. Next incremental firmware step: restore OLED status display, with its own bounded update rate and without adding Bluepad32 at the same time.
