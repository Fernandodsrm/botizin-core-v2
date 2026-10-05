# BOTIZIN WROOM: independent test base

ESP32-D0WD-V3, 4MB flash, CH9102 COM6, MAC 30:76:F5:90:67:E0.
Source derives from validated S3 0.0.10; the S3 source, manifest and devices are unchanged.
WROOM versions start at 0.0.1; internet manifest is main/wroom/manifest.json.

## Preserved recovery copy

Full 4194304-byte backup: wroom-v5-session-core-0.3-full-flash.bin.
SHA256 670ebe0e155e1e0cfe8169e55397e9e792e40bb23d846773d941a96643289e37.
Original firmware: FERNANDO LAB V5 SESSION CORE 0.3, Bluepad32 4.1.0, local AP FERNANDO-LAB.

The new minimal base provides HTTPS OTA, telemetry and diagnostic PING. It does not yet include the old OLED, button, LED or Bluepad32 application behavior. Hardware remains connected. The old behavior can be restored from the verified full backup.

## Credentials

Configure once over USB using provision_usb.py. Dedicated WROOM token only; never use the S3 token or account API key.
Configuration is one value in NVS namespace wroom-config. Wi-Fi driver persistence is disabled; Wi-Fi credentials are not compiled into cloud builds. Journal uses wroom-journal.

## Physical validation order

1. Download the successful cloud artifact. Close serial monitors.
2. Run install_usb.py with 0.0.1 directory, verified backup, esptool executable and COM6. The installer checks backup SHA, package SHA, USB adapter, MAC and 4MB flash before writing.
3. Run provision_usb.py; enter Wi-Fi and BOTIZIN-WROOM device token privately. Press RESET/EN when requested.
4. Open serial monitor at 115200. Confirm BOTIZIN WROOM V0.0.1, correct partition table, WIFI OK, OTA READY, TELEMETRY HTTP_200 and real ThingsBoard identity/version.
5. Publish 0.0.1 manifest as the initial up-to-date baseline, only after observing boot. No manifest has been published by this build workflow.
6. Separately publish 0.0.2 for physical OTA A→B; inspect received/flash hashes and boot partition.
7. Publish 0.0.3 for B→C, then power-cycle and require telemetry/PONG from a new boot.

Cloud checks confirm ESP32 chip ID 0, image size <= 0x140000 and all six partition rows. These checks cannot prove physical boot, power stability, TLS memory availability or internet connectivity. No erase-all operation is performed by the installer.
