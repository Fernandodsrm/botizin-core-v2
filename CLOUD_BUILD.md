# BOTIZIN cloud build

The `cloud-build` branch contains development firmware. Every firmware change
on that branch runs the ESP32-S3 build on GitHub Actions with core 3.3.12.
The notebook and board are not involved in compilation.

The workflow creates a versioned artifact with firmware.bin, SHA256, manifest,
ELF and build logs. It does not change the live main/manifest.json.

After successful compilation, verify the artifact against its exact source
commit and SHA. Only on Fernando's explicit publishing command, place the
binary at a new releases/VERSION/firmware.bin path and atomically publish its
manifest on main. Never replace an existing released version.

The ESP32 still uses the physically validated OTA process. It polls main's
manifest every minute, downloads HTTPS, validates size/SHA/flash readback,
switches slots and reboots. Version 0.0.8 adds only its version and test message.

The immutable baseline commit 8515aa3fcd6300efae0cb7eb1377ef270ba26c39 and
baseline-0.0.7-intocavel branch are preserved. Do not move that branch or edit
the 0.0.7 release files.
