# WROOM 0.0.5: remote boot and PONG; peer link not yet confirmed

Authenticated ThingsBoard observations after OTA, 2026-10-05 around 04:29–04:31 America/Sao_Paulo.

WROOM reports real firmware 0.0.5, boot eb4d558f6e28d929, app0 / ota_0 / address=0x00010000 / size=1310720, state VALID.
Correlated PONG received in 24.8 seconds:
command_id wroom-005-1791185379674-e3381919; firmware_version 0.0.5; boot_id eb4d558f6e28d929; uptime_seconds 53; running and boot partition app0 / ota_0 at 0x00010000; OTA VALID; reset SOFTWARE (3).

The WROOM peer reader reports s3_link HTTP_-1, empty s3_last_version, age -1: there is no confirmed local S3 report yet. Last S3 cloud report was 0.0.10 / VALID at timestamp 1791183991608 (04:06:31 local), about 24.5 minutes old when checked. A stale cloud report does not establish power state or prove an IP change. Verify S3 power, current local IP and local server availability before changing either firmware.

Physical navigation and S3 screen validation remain pending. Existing WROOM network access survived the peer connection failure. Manual OTA and LED relay stages remain gated on physical validation, as recorded in wroom/DEVELOPMENT_PLAN.md. S3 firmware/channel were not changed.
