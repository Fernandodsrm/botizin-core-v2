# 0.0.10 — one HTTPS PING/PONG command

Base: 0.0.9 source 98ba330e253cde60ddf2d028c7812e6e6ab98c46.
Controlled power-cycle confirmed by the user's real dashboard captures on 2026-10-05:
0.0.9 / VALID / POWERON (1) / uptime 25s / app1 ota_1 running and boot /
boot_id 4c667127cd42fa6a / sequence 1. Existing 0.0.9 binary SHA:
427e11ed9642b23fc594f3a36aa10659a2208f627ed09c3beeeb667c00c30c30.

OTA and telemetry function bodies are byte-identical to 0.0.9.
The only new module is firmware/ping_rpc.h, called after OTA and telemetry.
GET /api/v1/<device-token>/rpc?timeout=1000 every 15 seconds (first opportunity 30s).
HTTP 408/204 means no command; confirmed 408 against the live service.
Only method ping, integer request ID, nonempty command_id <=64 bytes, and fresh
issued_at_ms within 90s are accepted. No GPIO, Wi-Fi, flash or reboot operations.
Reply POST /rpc/{id}: PONG + original command_id + real version/boot/partitions/reset.
HTTPS builtin CA, no redirects, 5s transport timeouts, heap >=80k, incoming body <=1024.
Three failed transport operations pause only PING until reboot. OTA and telemetry continue.
Clients run sequentially; stack/heap/network remain shared, so physical validation is required.

Polling means seconds of latency, not an instant connection. No persistent RPC queue.
Firmware manifest on main remains 0.0.9 during preparation/build.
Acceptance after explicit release: 0.0.10 real telemetry, correlated real PONG,
continued minute OTA checks/telemetry, no panic, controlled power-cycle persistence.
