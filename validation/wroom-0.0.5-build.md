# WROOM 0.0.5: navigation and read-only S3 link

Source fb065e04e159ce10d6c00b44d66a0f96963f48e1. GitHub Actions 37277412354 succeeded.
Image 1,218,384 bytes <= preserved 1,310,720-byte OTA slot. SHA-256 b57d740f4f127128b268454718b220b4ffa327da2f68a7eca97eae8cfab3ab8a. Classic ESP32 chip header and partition table byte-identical to 0.0.4 verified.

UP27/DOWN33 switch WROOM/S3, OK25 switches status/details and requests a fresh S3 poll, BACK32 returns to WROOM. Active-low inputs match original V5 Config.h. Debounced ISR records press only, loop handles it. No button starts firmware update or drives LED in this version.

Read-only peer: GET http://192.168.0.36/status, bounded body 8192 bytes, connect timeout 350ms, HTTP timeout 400ms, body deadline 750ms. Poll every 10s on S3 screen and 30s otherwise. Firmware uploads and reboot take priority. Peer validation requires BOTIZIN CORE identity, 16MB flash, WIFI OK and complete current fields. Parser does not read persisted OTA journal as current state. Host/CI tests cover valid report, CRLF, wrong identity, uptime overflow, missing fields and journal-only fields. Display marks reports stale after 30s and shows age. This checks a known local endpoint, not cryptographic peer authentication; no writes to S3.

Telemetry includes s3_link, s3_last_version, s3_last_seen_age_seconds. PING widget accepts 0.0.3/0.0.4/0.0.5 and deployment was read back. S3 channel unchanged, GitHub OTA60s/TB60s/PING15s retained. Manual OTA confirmation and S3-to-WROOM LED remain future stages requiring physical validation of navigation/link first.

Physical 0.0.5 validation pending at publication. Known S3 IP may need correction if DHCP changed it. Previously validated 0.0.4 code/release references unchanged.
