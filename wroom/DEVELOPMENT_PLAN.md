# Central Lab: agreed development order

Working baselines preserved: WROOM 0.0.5 (OLED, physical navigation, real S3 local status, telemetry and correlated PONG), S3 0.0.10.

1. WROOM 0.0.5: physical navigation buttons and read-only S3 status on OLED. No S3 firmware change. Confirm actual buttons, screen switch, S3 current state, WROOM telemetry and PONG.
2. Add authenticated, allowlisted OTA check/confirmation commands for each board. Share one candidate-validation/apply path between cloud panel and physical controls; bind confirmation to target board, version and SHA, with expiry. Preserve SHA verification, journal, slots, provisioning and working communication.
3. Show progress and post-boot result. Acknowledged request is not installed firmware. During a WROOM reboot the shared screen disappears; it must return with actual running version. During S3 updates WROOM needs reported progress and freshness.
4. Test remote recovery and command reception, including temporary ThingsBoard failure. Only then disable periodic GitHub OTA lookup. Command reception remains enabled; automatic recovery mode can be requested through an available authenticated channel or physical controls. A complete loss of all connectivity cannot be repaired by a remote command.
5. Add S3 → WROOM relay for a first idempotent LED command (explicit desired ON/OFF, no toggle), per-peer credentials, command ID, freshness/expiry and correlated acknowledgement from WROOM. Show confirmed state in both panel and screen. Test wrong credentials, duplicate/expired requests and peer outage.

Read-only first link uses existing local HTTP status server at S3 last-known address 192.168.0.36. A changed address must be diagnosed; no automatic scanning or assumption of success. Same LAN connectivity can survive WAN loss; router/power loss stops this LAN link. A cellular hotspot/router is a future shared uplink, not included in current firmware.

Do not change partition layout, erase NVS, install WROOM image on S3, or disable the verified OTA channel before its replacement path is proven.

2026-10-05: user explicitly keeps automatic OTA enabled for now on both boards. WROOM 0.0.6 adds manual query/confirmation and OLED download/hash/reboot progress. Manual consultation has a five-minute reservation, after which automatic installation resumes; no automatic-off command is included. S3 remains 0.0.10. Full manual installation, physical controls and S3 progress/confirmation remain validation gates before any later removal of periodic checks.
