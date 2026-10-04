# BOTIZIN 0.0.9 — HTTPS diagnostic report

New function sendTelemetry posts to ThingsBoard after the existing OTA check returns.
No new broker, remote command, Wi-Fi setting, partition table, bootloader or rollback policy.
The 16 existing diagnostic/OTA functions are byte-identical to cloud-build 0.0.8.
The validated static 4096-byte OTA buffer is preserved.

Reports firmware_version, running/boot/next partitions, reset_reason, ota_state,
uptime, boot_id, sequence, origin=ESP32_REAL, simulated=false and persisted OTA journal.
The journal retains SOURCE_VERSION and received/flash SHA evidence. No simulated reports
are interpreted as board confirmation. Receiving 0.0.9 proves 0.0.9 only; SOURCE_VERSION
in the real journal can establish whether the source was 0.0.7 or 0.0.8.

First telemetry opportunity is 20 seconds after setup, then every 60 seconds.
Each HTTPS client has 5-second connect/read/handshake timeouts. No redirects or insecure TLS.
No report while an OTA/upload/reboot is pending. Low heap skips telemetry.
Three consecutive failed requests pause telemetry until next boot, without changing OTA state.
These controls reduce risk; shared loop/heap/network still means isolation is not absolute.
No physical safety/health rollback guarantee: existing Arduino startup confirmation stays.

GitHub repository Secret BOTIZIN_TB_DEVICE_TOKEN injects the DEVICE credential at compile time.
The account API key is forbidden in firmware. The generated header is ignored and not uploaded.
A missing secret produces a VALIDATION-ONLY artifact with publishable=false and dummy token.
That artifact must NEVER be published. A release requires publishable=true, successful cloud
compile, exact source commit, binary SHA/size verification and review before manifest changes.
The binary contains a device credential, so a publicly downloadable image does not keep that
credential confidential. It is scoped to this test device; account API keys remain external.

No release path, live manifest or baseline-0.0.7-intocavel changes in this preparation commit.
Physical acceptance: real report, correct version/partitions/VALID, no recurring reset,
continued GitHub OTA polls, power-cycle retention; receiver failure must not block future OTA.
