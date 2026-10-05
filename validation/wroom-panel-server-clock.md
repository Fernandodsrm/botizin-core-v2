# WROOM panel server clock correction

2026-10-05. User panel showed telemetry at 07:30:12 as 3m15s old around 07:31. Exact browser offset was not independently measured. Cloud check at 10:31:54 UTC found firmware 0.0.7 telemetry only 42.1s old, same boot cf0206162ef664af, OTA VALID, S3 link OK, sequence 10. Earlier direct RPC PONG also confirmed the receiver.

All three WROOM controllers previously relied on browser Date.now() for freshness; RPC issued_at_ms did too. Firmware accepts a narrow issuance window, so browser skew can both falsely disable buttons and reject commands. This is a verified code vulnerability; the user's specific browser offset remains unmeasured.

Controllers now fetch ThingsBoard's /api/dashboard/serverTime using the logged-in widget HTTP client, anchor epoch to performance.now(), and resync every minute. A sample older than 120s blocks commands. Round trips >10s are rejected; full round-trip latency is added conservatively for age. Telemetry older than 180s or implausibly future-dated remains ineligible. Candidate expiry and PONG duration use monotonic elapsed time. No embedded credentials.

Validation: existing PING and OTA controller tests pass; new tests cover browser +140s/-300s skew, server-based RPC issuance, real stale and future data, and clock expiry. Server-time endpoint returned a numeric epoch in the live account. Firmware, manifest, partition layout, NVS and automatic OTA are unchanged. Browser reload/PING is still required to verify the user's end-to-end UI.
