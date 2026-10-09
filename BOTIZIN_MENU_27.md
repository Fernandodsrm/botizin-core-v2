# Botizin Menu 0.0.27

The standalone WROOM Menu starts on an autonomous 128x64 character. Eight
procedural expressions vary in a shuffled order every 2.2–4.8 seconds. Gaze
changes, easing, curiosity, smiling eyes, surprise, thoughtful asymmetry,
focus, blinks, double blinks and winks keep it active without an idle sleep.
Eyebrows stay in the OLED yellow band; eyes and mouth stay in the blue band.

The first physical navigation press opens the root Menu and is consumed.
After 30 seconds without a press it returns to the character. Uploads, reboot,
queued OTA/module operations, a valid OTA candidate and open confirmation
screens inhibit the idle return. No character button actions, speech or
controller reactions are introduced in this release.

Rendering targets 16 frames/s in a dedicated 4096-byte FreeRTOS task. Existing
network requests remain sequential on loopTask. A mutex serializes the entire
OLED framebuffer/I2C transaction; only atomic mode/counters are shared with
the renderer. An I2C fault disables the display without changing flash or boot.
Status and telemetry report face_frames and face_max_gap_ms for hardware QA.
The target frame rate must be verified on the real OLED, including cloud polls.

The drawing uses existing Adafruit GFX/SSD1306 primitives and no bitmap bank or
additional library. Behavioral references: [FluxGarage RoboEyes](https://github.com/FluxGarage/RoboEyes)
and [Adafruit OLED guide](https://learn.adafruit.com/monochrome-oled-breakouts/overview).
The face engine is independently implemented; no RoboEyes source was copied.

## OTA and module compatibility

Both original 0x140000 OTA partitions are preserved. Installing the new Menu
from Menu 22 overwrites the other slot, previously containing PS4/servo 26.
Historical release binaries remain archived in GitHub.

Module 28 preserves the previous PS4/servo behavior and accepts verified return
anchors for Menu 22 or 27. Menu 27 recognizes module 28; it intentionally does
not start older modules that reject a Menu 27 return anchor. Module 28 is stored
without changing the live manifest until Menu 27 is physically confirmed.

The existing compact ThingsBoard widget is updated to accept Menu 27 and
identify PS4 releases through 28. Reimport the updated widget definition into
the same custom widget; the dashboard/device/layout are preserved.

## Physical validation

1. Install Menu 27 through the current Menu OTA, then confirm /status shows 27.
2. Observe the face for a minute and during a cloud telemetry/RPC poll.
3. Press any button: it must open the Menu without selecting an item.
4. Leave ordinary navigation idle for 30 seconds: the character returns.
5. Consult GitHub/open OTA confirmation: the character must not cover the process.

Host tests cover navigation/OTA guards, timeout/rollover and lit geometry and
all expressions across 100 random seeds. Cloud compilation additionally
checks the classic ESP32 image, exact preserved partition table and slot fit.

## Verified packages

Menu 27: 1,276,448 bytes; OTA margin 34,272 bytes. SHA256:
`dd949d04edbbbfa42455f0920303f23c2e089ba3e84ae4e5db3e6c9e4b0385d7`.
PS4 28: 761,344 bytes. SHA256:
`3e6c7be2fbb17c207a4023fefabb9f4d44a46dc0690986f586ce24de649aa801`.
Both builds use exact source `8a461051b350f03b65c15b4b70f4cbf211d125ad`.
Build runs 37976443043 (Menu) and 37976442937 (module) succeeded. Storage run
37976808971 succeeded; public downloads were independently checked for size
and SHA256. The live manifest now offers Menu 27 only. Physical validation
of boot, animation smoothness, buttons, idle return and module return remains
pending the user's hardware test.
