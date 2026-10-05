# WROOM physical USB baseline 0.0.1

User serial capture, 2026-10-05 (America/Sao_Paulo).

ESP32-D0WD-V3, MAC 30:76:F5:90:67:E0, 4MB, no PSRAM. Existing six-row partition table preserved. Running and boot: app0 0x10000, size 1310720; next: app1 0x150000. Initial USB image OTA state UNDEFINED (not claimed VALID). CONFIG_LOADED after physical RESET/EN. WIFI OK, IP 192.168.0.38, OTA READY. Two telemetry POST results HTTP_200. WROOM manifest HTTP_404 before channel publication.

Next test: publish WROOM 0.0.2 only; require download and flash SHA equality, boot into app1, runtime version 0.0.2, persisted Wi-Fi and fresh real telemetry. OTA and final power-cycle are not yet proven. S3 0.0.10 channel is unchanged.
