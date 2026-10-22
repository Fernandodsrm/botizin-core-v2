#!/usr/bin/env python3
"""OTA-01: independent checks of the REAL S3 0.0.17 source. No flashing."""
import pathlib
import re
import subprocess
import tempfile
import textwrap
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
FIRMWARE = (ROOT / "firmware/firmware.ino").read_text()
VERSION = (ROOT / "firmware/version.h").read_text()
CONFIRM = (ROOT / "firmware/ota_confirmation.h").read_text()
CONFIG = (ROOT / "firmware/internet_config.h").read_text()
CORE_PATH = ROOT / "tests/reference-arduino-core-3.3.12-misc.c"
if not CORE_PATH.exists():
    raise SystemExit("ERROR: Missing pinned upstream core source. Do not guess rollback behavior.")
ARDUINO_CORE = CORE_PATH.read_text()

def section(start, end):
    return FIRMWARE.split(start, 1)[1].split(end, 1)[0]

class Ota01RealCodeTests(unittest.TestCase):
    def test_01_reference_firmware_identified(self):
        self.assertIn('BOTIZIN_VERSION "0.0.17"', VERSION)
        self.assertIn('#include "ota_confirmation.h"', FIRMWARE)
        self.assertIn('#include "mqtt_observer.h"', FIRMWARE)
        print("TEST 01 OK: S3 0.0.17 is the studied source")

    def test_02_real_semver_comparison_compiled_and_executed(self):
        # Extract the ACTUAL implementation, not a hand-written model.
        function = "bool newerVersion(" + section("bool newerVersion(", "\nvoid stopInternet(")
        cases = [
            ("0.0.16", False), ("0.0.17", False), ("0.0.18", True),
            ("0.0.19", True), ("0.1.0", True), ("1.0.0", True),
            ("0.0.0", False), ("banana", False), ("0.0.17BAD", False),
            ("0.0.18BAD", False), ("0.0", False),
        ]
        tests = "\n".join(
            '  if (newerVersion("%s") != %s) { std::cerr << "FAIL version %s\\n"; return 1; }' %
            (v, "true" if expected else "false", v)
            for v, expected in cases
        )
        program = (
            '#include <string>\n#include <cstdio>\n#include <iostream>\n'
            '#include "firmware/version.h"\nusing String=std::string;\n'
            + function + '\nint main() {\n' + tests +
            '\n std::cout << "SEMVER_REAL_FUNCTION_PASS %d cases\\n"; }\n' % len(cases)
        )
        with tempfile.TemporaryDirectory() as tmp:
            src = pathlib.Path(tmp) / "ota01_versions.cpp"
            exe = pathlib.Path(tmp) / "ota01_versions"
            src.write_text(program)
            subprocess.run(["g++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
                            "-I", str(ROOT), str(src), "-o", str(exe)], check=True)
            result = subprocess.run([str(exe)], check=True, capture_output=True, text=True)
        print("TEST 02 OK:", result.stdout.strip())

    def test_03_exact_manual_confirmation_helper_executed(self):
        exe = pathlib.Path(tempfile.mkdtemp(prefix="ota01-check-")) / "ota01_confirmation"
        try:
            subprocess.run(["g++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
                            str(ROOT / "tests/ota_confirmation_test.cpp"), "-o", str(exe)], check=True)
            result = subprocess.run([str(exe)], check=True, capture_output=True, text=True)
        finally:
            exe.unlink(missing_ok=True)
            exe.parent.rmdir()
        self.assertIn("OTA_CONFIRMATION_TESTS_OK", result.stdout)
        self.assertIn("strcmp(boot, expectedBoot)", CONFIRM)
        print("TEST 03 OK:", result.stdout.strip())

    def test_04_automatic_path_needs_new_version(self):
        check = section("void checkInternetOTA(bool manual)", "\nvoid installInternetOTA(")
        self.assertIn('if (targetVersion == BOTIZIN_VERSION)', check)
        self.assertIn("if (!newerVersion(targetVersion))", check)
        self.assertIn("installInternetOTA(targetVersion, targetURL, targetSHA, targetBytes);", check)
        self.assertIn('stopInternet("manifest version is invalid or older")', check)
        print("TEST 04 OK: same version => no install; older version => STOPPED; newer => install")

    def test_05_manifest_is_not_entire_github(self):
        self.assertIn("botizin-core-v2/main/manifest.json", CONFIG)
        self.assertIn("botizin-core-v2/main/releases/", CONFIG)
        check = section("void checkInternetOTA(bool manual)", "\nvoid installInternetOTA(")
        self.assertIn("BOTIZIN_MANIFEST_URL", check)
        self.assertIn("targetURL.startsWith(BOTIZIN_FIRMWARE_PREFIX)", check)
        self.assertIn('String(board->valuestring) == "esp32s3-n16r8"', check)
        print("TEST 05 OK: only permitted manifest and release prefix")

    def test_06_sha_size_and_flash_readback_present_in_real_path(self):
        self.assertIn("Update.begin(expectedBytes, U_FLASH)", FIRMWARE)
        self.assertIn("Update.setSHA256(expectedSha.c_str())", FIRMWARE)
        upload = section("void finishUpload()", "\nvoid handleUpload()")
        self.assertIn("receivedSha != expectedSha", upload)
        self.assertIn("Update.end(false)", upload)
        self.assertIn("hashPartition(destination, expectedBytes, storedSha)", upload)
        self.assertIn("storedSha == expectedSha", upload)
        self.assertIn("esp_ota_set_boot_partition(originalBoot)", upload)
        self.assertLess(upload.index("Update.end(false)"), upload.index("hashPartition(destination"))
        print("TEST 06 OK: size/hash received and SHA readback in the real update path")

    def test_07_real_slot_requirements(self):
        setup = section("void setup()", "\nvoid loop()")
        self.assertIn("a->address == 0x10000 && b->address == 0x310000", setup)
        self.assertIn("a->size == 0x300000 && b->size == 0x300000", setup)
        self.assertIn("data->address == 0xe000 && data->size == 0x2000", setup)
        print("TEST 07 OK: running firmware asserts two 3MiB OTA slots")

    def test_08_existing_firmware_does_not_defer_rollback_validation(self):
        self.assertNotRegex(FIRMWARE, r"\bbool\s+verifyRollbackLater\s*\(")
        self.assertNotRegex(FIRMWARE, r"\bbool\s+verifyOta\s*\(")
        self.assertIn("bool verifyOta() {", ARDUINO_CORE)
        self.assertRegex(ARDUINO_CORE, r"bool verifyOta\(\)\s*\{\s*return true;")
        self.assertRegex(ARDUINO_CORE, r"bool verifyRollbackLater\(\)\s*\{\s*return false;")
        init = section_from_core("void initArduino()", "esp_log_level_set")
        self.assertIn("if (!verifyRollbackLater())", init)
        self.assertIn("esp_ota_mark_app_valid_cancel_rollback()", init)
        self.assertIn("esp_ota_mark_app_invalid_rollback_and_reboot()", init)
        print("TEST 08 OK: Arduino core validates new image immediately by default (not after Wi-Fi/MQTT)")

    def test_09_no_auto_rollback_after_mqtt_loss(self):
        # The current S3 firmware has no rollback API usage; Arduino core handles initial boot.
        self.assertNotIn("esp_ota_mark_app_invalid_rollback_and_reboot", FIRMWARE)
        self.assertNotIn("esp_ota_mark_app_valid_cancel_rollback", FIRMWARE)
        self.assertNotIn("verifyRollbackLater(", FIRMWARE)
        print("TEST 09 OK: no post-boot fallback on connectivity failure in S3 application")

    def test_10_manual_five_minutes_is_not_rollback(self):
        c = section("void cancelOtaCheck()", "\nbool peerOtaBusy")
        self.assertIn("otaManualUntil = millis() + 300000", c)
        self.assertIn('manualOtaStatus = "CANCELLED_AUTO_IN_5MIN"', c)
        self.assertNotIn("esp_ota", c)
        print("TEST 10 OK: manual 5-minute window delays automatic install, NOT rollback")

    def test_11_transient_wifi_and_fatal_ota_errors_are_distinct(self):
        check = section("void checkInternetOTA(bool manual)", "\nvoid installInternetOTA(")
        self.assertIn('internetStatus = "WAITING_FOR_WIFI"', check)
        self.assertIn('internetStatus = "WAITING_FOR_NTP"', check)
        stop = section("void stopInternet(const String &reason)", "\nvoid checkInternetOTA(")
        self.assertIn("internetStopped = true", stop)
        loop = section("void loop()", "\n// NEVER_MATCH_THIS_SENTINEL") if False else FIRMWARE.split("void loop()", 1)[1]
        self.assertIn("!internetStopped", loop)
        print("TEST 11 OK: some errors pause OTA until reboot; Wi-Fi/NTP wait can retry")

    def test_12_mqtt_and_ota_are_separate_channels(self):
        mqtt = (ROOT / "firmware/mqtt_observer.h").read_text()
        self.assertIn("botizinMqtt.publish(", mqtt)
        self.assertNotIn("botizinMqtt.subscribe(", mqtt)
        self.assertIn("checkInternetOTA(false)", FIRMWARE)
        self.assertIn("botizinMqttLoop();", FIRMWARE)
        print("TEST 12 OK: MQTT publishes state; OTA checks GitHub via HTTPS independently")

def section_from_core(start, end):
    return ARDUINO_CORE.split(start, 1)[1].split(end, 1)[0]

if __name__ == "__main__":
    unittest.main(verbosity=2)
