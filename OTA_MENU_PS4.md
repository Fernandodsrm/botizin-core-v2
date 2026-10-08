# Menu OTA and standalone PS4 — 2026-10-08

Authorized first delivery: compact mobile ThingsBoard dashboard, three OLED root entries (Connections, Diagnostics, Updates), explicit manual OTA with optional persistent automatic-check policy, and standalone Bluetooth PS4 -> GPIO26 LED.

Source compiled: c47cd9dc471c2d6f26fe6531a64452063191f348.
Menu 0.0.18 build 37818152711: 1,293,232 bytes / SHA256 26c9216c767ffb4490abe3f31a63ba0e1f933ea6f3c687f46eb71c47acb31150.
PS4 0.0.19 build 37818152834: 755,024 bytes / SHA256 ff5f994c079f9a15eb3439abe9613509a75e9d594d9e40cb56618bd72e69c9f1.
Both CI builds succeeded, image chip/partition/size checks passed. Menu static RAM 68,224; PS4 static RAM 88,076 bytes. These are linker numbers, not live heap or CPU utilization.

## Return and radio
Menu records its image digest and partition address in a single verified NVS value before updating the inactive slot. PS4 checks the partition layout, recorded Menu version (18), actual image digest, boot selection and readback before enabling Bluetooth or LED actions. It selects Menu as next boot before Bluetooth starts. Physical BACK32 turns LED off, shows return feedback and restarts into Menu. Power cycling PS4 also targets Menu. No fixed ten-minute timer in the normal module. A failure before the guard arms is not guaranteed remotely recoverable; PS4 has no Wi-Fi service and must not be installed directly from old firmware without the Menu anchor.

Menu retains cloud/Wi-Fi service. PS4 has no Wi-Fi/HTTP/TLS/OTA application code and does not start Wi-Fi. Configure the LED button in Menu; it is stored separately in ps4-config and read by PS4. The cloud dashboard cannot control or observe the module during Bluetooth-only use. OLED/physical return are the confirmation path. The first trial has no motors or servo outputs.

Menu can start an already installed PS4 only after verifying the previous installation journal's target version, length and flash digest against the inactive slot. Boot binding is mandatory for cloud launch. Automatic firmware queries must be disabled, proved and persisted before offering module 19; commands/telemetry continue in Menu. Three transient cloud failures schedule another attempt after 60 seconds instead of stopping until reboot.

## Dashboard
The existing WROOM dashboard ID is preserved. Original layout is in panel-mobile/dashboard-before.json. A new mobile widget replaces the three tall cards; original widget types remain available. No device/account keys are embedded. Server-clock anchoring, correlated RPC replies, candidate/hash/boot binding, expiry and post-reboot confirmation remain. Buttons support deployed 15 and Menu 18; feature-specific policy/configuration/start buttons require 18. Controller tests exercise old-version rejection, current-version acceptance, correlation, install binding and automatic-off replies.

## Scope still pending physical validation
No physical PS4 pairing/LED test has been claimed. No live Bluetooth response latency has been measured. Cross-core Arduino compatibility is checked by each build, but booting the Bluepad32-core image under the existing bootloader and NVS remains a physical test. S3 firmware is unchanged and no general four-slot environment manager is claimed in this delivery. The OLED can inspect the WROOM environment and start its installed PS4. Full S3 slot catalogs and arbitrary modules are later extensions.
