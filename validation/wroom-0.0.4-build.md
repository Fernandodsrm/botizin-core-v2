# WROOM 0.0.4 — optional OLED status

Source commit: 354c639dcdc10efbbcbef7254e349171d5900e27.
GitHub Actions run: 37274951732 (success).
Image: 1,210,528 bytes; preserved OTA capacity: 1,310,720 bytes.
SHA-256: 5abe41a8e67bab9f1679aec645f25e08e2f22731895e15af3492312280ecc752.

Classic ESP32 image header verified. Compiled partition table byte-identical to 0.0.3. Wi-Fi, OTA, provisioning, telemetry and PING source unchanged; additions are OLED initialization/refresh and status diagnostics.

SSD1306 128x64, address 0x3C, SDA21/SCL22. I2C transaction timeout 20ms, refresh at most once per second, skips uploads and scheduled reboot. Missing screen or failed probe disables only OLED until next boot. Libraries pinned: SSD1306 2.5.17, GFX 1.12.6, BusIO 1.17.4.

WROOM PING widget accepts fresh real telemetry from 0.0.3 and 0.0.4. Correlation, busy gate and timeout behavior tested; deployed widget read back.

Build verification is not physical OLED validation. Physical boot, Wi-Fi/telemetry, PONG and OLED remain to be observed after publication. S3 channel and 0.0.3 preservation branches unchanged.
