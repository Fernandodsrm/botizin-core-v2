# WROOM 0.0.5: physical navigation and S3 local link verified

User photos from 2026-10-05 04:34–04:35 America/Sao_Paulo and supplied S3 /status output confirm:
- OLED WROOM 0.0.5 [1/2], WiFi OK, IP 192.168.0.38, app0 / OTA PRONTO, current Internet OTA, TB HTTP_200.
- WROOM detail view: PING READY, SOFTWARE (3), OTA VALID, S3 OK.
- OLED S3 0.0.10 [2/2], CONTATO RECENTE, report age 2 seconds, IP 192.168.0.36, OTA VALID, uptime 91 seconds at report.
- S3 detail view: report age 7 seconds, app0 / ota_0, POWERON (1), UP_TO_DATE 0.0.10.
- Serial navigation transitions WROOM/S3 status and detail, with S3_LINK OK / last_version=0.0.10 after earlier connection failures. This confirms recovery without another firmware install; root cause of earlier loss is not asserted.

Authenticated ThingsBoard read, report timestamp 1791185917897:
- WROOM firmware_version 0.0.5, origin ESP32_REAL, simulated false.
- boot_id eb4d558f6e28d929, running app0 / ota_0 at 0x00010000, OTA VALID.
- s3_link OK, s3_last_version 0.0.10, s3_last_seen_age_seconds 1.

User S3 /status: BOTIZIN CORE 0.0.10, IP 192.168.0.36, uptime 289, POWERON (1), run and boot app0 / ota_0 at 0x00010000, size 3145728, VALID, WIFI OK, OTA READY, UP_TO_DATE 0.0.10, TELEMETRY HTTP_200. Persisted 0.0.9 → 0.0.10 journal retains matching expected/received/flash SHA b0ad415a8dee35ac647b26b24260983040ea2a969cb577a9162587464c72abf9.

Together with the prior correlated 0.0.5 PONG, navigation, optional OLED and read-only local S3 status are now physically validated. This is status exchange, not S3-to-WROOM actuation or manual OTA confirmation. Periodic GitHub OTA remains enabled until its replacement is tested.

Preserved source commit fb065e04e159ce10d6c00b44d66a0f96963f48e1. Previously preserved 0.0.4 and S3 source/channel remain unchanged. Preservation branches are stable references, not enforced branch protection.
