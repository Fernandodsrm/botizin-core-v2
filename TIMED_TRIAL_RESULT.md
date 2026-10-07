# S3 timed OTA trial — 2026-10-07

Status: compiled and offered; execution and two-minute return NOT demonstrated on hardware.

## Compiled image

- Source: aa9e3340e50bb6f7a2ff2ba38e3418f1a89610b8, based on the existing cloud-build 0.0.13.
- Version: 0.0.14; application BIN: 1,164,880 bytes; slot: 3,145,728 bytes.
- SHA256: 553591a8ceab9f12b01a873d496cc6d55d86993af867ba569fff58dd09fbf66a.
- Build passed: https://github.com/Fernandodsrm/botizin-core-v2/actions/runs/37577069359
- Native guard tests passed for both slot directions, bad image hash/read, invalid partitions, timer failures, boot-selection failure and readback mismatch. These tests mock hardware APIs and do not establish real boot recovery.

The trial validates the previous slot against the exact 0.0.13 BIN SHA and length, arms an ESP timer for 120 seconds before networking, selects the previous slot as next boot, and blocks OTA writes while running. It does not change partitions or bootloader. Crash recovery before arming is not guaranteed.

## Offer observations (UTC)

- First offer: 05:39:09; baseline manifest restored 05:40:40. Workflow succeeded. No experimental boot observed.
- An intermediate retry was rejected before offering because the GitHub Contents response omitted inline content for the existing BIN over 1 MiB. Publisher was corrected to compare size and Git blob identity.
- Second actual offer: 05:45:20; baseline restored 05:46:51. Workflow succeeded: https://github.com/Fernandodsrm/botizin-core-v2/actions/runs/37577878405
- Telemetry at 05:49:14 remained 0.0.13, boot c2347bcf18d55a0e, uptime 42,443 seconds, 708 Git queries and 0 recorded Git failures. No observed reboot or trial status.
- The public raw manifest endpoint advertised Cache-Control max-age=300. A cache can explain a short offer being missed, but the exact manifest response received by the device was not captured; cause is unconfirmed.

Main manifest was verified restored to 0.0.13 with SHA db682f24db7af0624f4ed0603738d6536643ce7cb5e34eb08a5e99ff4fc8adab. No WROOM offer changed. Firmware execution/return remains pending a controlled delivery test that handles caching and prevents repeated offers.

## Menu implication

Only the running application consumes application RAM/CPU. The inactive slot still occupies flash. Reset clears runtime state while NVS configuration persists. With only two slots, an additional OTA can overwrite the inactive Menu; permanently retaining Menu plus reusable OTA slots requires a separately planned partition layout, or downloading Menu again. This experiment does not establish that permanent layout.
