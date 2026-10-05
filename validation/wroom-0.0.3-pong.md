# WROOM 0.0.3 panel and real PING validation

2026-10-05, America/Sao_Paulo. No firmware publication or device flash operation.

ThingsBoard real telemetry verified before panel creation: firmware_version 0.0.3, origin ESP32_REAL, simulated false, board esp32-wroom-4mb, device_id 30:76:F5:90:67:E0, contact younger than 180 seconds.

Created a dedicated private WROOM dashboard and two own widget types. Readback confirmed configuration and private assignment. Existing S3 dashboard configuration was read before and after and remained identical. UI source and widget tests preserved in branch wroom-panel-0.0.3, directory ui/wroom. Widget tests cover version gate, busy gate, rejection of uncorrelated PONG and timeout.

Authenticated diagnostic RPC ping (persistent false, timeout 30000ms) returned a correlated ESP32_REAL PONG in 19.7 seconds. command_id wroom-ping-1791182721451-01a4dc2f, request_id 0, firmware_version 0.0.3, boot_id 3852912ef418cdea, uptime_seconds 457. Running and boot partitions app0 / ota_0 at 0x10000, size 1310720, OTA VALID, reset POWERON (1).

This proves remote round-trip communication for the WROOM without changing firmware. Native dashboard rendering and its button await the user's browser test. No credentials are recorded here.
