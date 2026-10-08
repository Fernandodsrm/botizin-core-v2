# Menu OTA 20 and PS4 21 — 2026-10-08

## Authorized delivery
WROOM stays the physical interface. OLED root has Connections, Diagnostics and Updates; placeholder Controls and Help are removed. Existing WROOM/S3 connection and OTA pages remain. Updates -> WROOM -> View slots / PS4 inspects the running Menu and the verified inactive PS4, and requires a second physical confirmation before starting it. This delivery does not claim a general arbitrary four-slot manager or S3 slot launch.

Existing WROOM ThingsBoard dashboard ID 4a411320-c088-11f1-b930-ad60e0953111 is preserved, with a new compact widget and independent S3 cloud datasource. Old tall widgets and backup layout remain available. UI has real version/IP/age/slot, S3 cloud contact, inter-board link, explicit OTA consultation/result/install/cancel, automatic-policy controls, saved PS4 LED-button choice and launching an installed PS4. Policy/configuration/start controls require Menu 20. Server-clock anchoring, command correlation, candidate/hash/boot binding and expiry are retained. UI tests cover mixed device data without overwriting WROOM identity, version gates, incorrect replies and automatic-off acknowledgement.

## Build proof
Exact compiled source 0df8098416699ecb8f941f33bb17ea1b76b1b70c.
Menu 20 build 37818969872 succeeded: 1,293,232 bytes, SHA256 f955a7a2f96cd12b27168650f2bc3a4ddabb3ca609b8dac38f8e6e3f7696a39d, static RAM 68,224 bytes.
PS4 21 build 37818969869 succeeded: 755,024 bytes, SHA256 eb929d8cc58ccbbf90813df3837b267fbacc5bda29c04902c118fec397bd50e3, static RAM 88,076 bytes.
Preserved classic ESP32 chip and both 1,310,720-byte OTA slots were checked. Public downloads independently matched their size and SHA. Static RAM/linker numbers are not live heap or CPU usage.
Initial packages 18/19 were stored before an additional navigation test found a root wrap error after leaving the WROOM OTA page. Offer was restored to 15 within the reserved manual window; no installation was confirmed, and later fresh telemetry still showed the same 15 boot. Files were preserved and corrected new versions 20/21 were built. Root wrapping is tested explicitly after returning from the WROOM OTA page.

## Installation and recovery
The Menu retains Wi-Fi, existing HTTPS/local OTA writers and cloud commands. Three transient telemetry/RPC failures retry after 60 seconds instead of permanently pausing until reboot. Automatic GitHub checks default to the existing enabled state; a validated RPC stores the policy in NVS. They are disabled only after proving the manual OTA path. This does not disable the cloud receiver or telemetry.
Menu stores its current partition address, image digest and version in one read-back-verified NVS value before writing the inactive slot. PS4 verifies Menu 20, the actual image digest, preserved partition layout, boot selection and boot readback before enabling Bluetooth or LED actions. Menu is selected as the next boot before Bluetooth starts. BACK32 turns LED off, shows 750 ms return feedback and restarts into Menu. A power cycle after the return guard arms also targets Menu. No ten-minute expiry timer in the normal PS4 module. A failure before arming is not guaranteed recoverable remotely. Do not install PS4 directly from 15 or other old firmware; it requires Menu 20 to create the return anchor.
Menu launches a previously installed PS4 only after matching the installation journal's version/length/flash SHA against the actual inactive slot; cloud launch is also bound to the current Menu boot. Future automatic offers must not overwrite the reserved Menu during module use.

## First physical test
In Menu, save one DualShock face button for LED (default cross / bit 1), then install PS4 21 through explicit OTA. PS4 has Bluetooth/OLED/LED/physical return and no Wi-Fi application service. Pair with SHARE + PS; chosen button holds GPIO26 LED on, release switches it off. Disconnect or a one-second input timeout turns LED off. OLED shows connection, buttons and analog axes. BACK32 returns to Menu; afterward the cloud dashboard can receive fresh Menu telemetry again. During PS4 the WROOM cloud contact expires by design; S3 direct cloud status remains visible. No motor/servo is driven by this first trial.

Physical PS4 pairing, LED behavior and return are not claimed before user confirmation. Live Bluetooth latency and runtime heap are not yet measured; compiled compatibility cannot prove boot under the existing bootloader/NVS on hardware. S3 firmware is unchanged in this first delivery.

## Observed deployment result
Manual OTA: WROOM 15 / app0 / boot d47bddebd133f48c -> Menu 20 / app1 / new boot 254a187310d13b06. Actual flash journal confirms 1,293,232 bytes, Update.end TRUE and exact SHA256 f955a7a2f96cd12b27168650f2bc3a4ddabb3ca609b8dac38f8e6e3f7696a39d; new Menu reports VALID. Correlated AUTOMATIC_SAVED RPC reports automatic_enabled false. Correlated PS4_CONFIG_SAVED reports button 1. PS4 21 is offered and the real Menu has returned a matching, unexpired candidate; it has not been installed or physically tested by this turn.

Name discovery did not resolve S3 during the first minutes of Menu 20 boot. A temporary fallback was saved to preserve Connections; the subsequent real replies resolved 192.168.0.9 with source MDNS. The fallback remains 192.168.0.9: automatic review rejected its removal as contrary to the reserve-address plan. This does not affect PS4. Reboot discovery latency and the physical router/IP-change test are still open; no definitive instantaneous discovery guarantee is claimed. S3 remains 16, boot dce0fbc2a795f7ef.
