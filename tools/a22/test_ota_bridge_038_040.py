#!/usr/bin/env python3
"""Offline regression checks. No network access and NO flashing."""
import hashlib
from pathlib import Path
import tempfile
import unittest
from unittest import mock
import ota_bridge_038_040 as b

class BridgeTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.cache = Path(self.tmp.name)
        self.mockcache = mock.patch.object(b, "CACHE", self.cache)
        self.mockcache.start()
        self.addCleanup(self.mockcache.stop)
        self.known38 = (
            "BOTIZIN WROOM V0.0.38\nFLASH_BYTES: 4194304\n"
            "OTA: READY\nOTA_AUTOMATIC_ENABLED: NO\n"
            "RUNNING_PARTITION: app0 / ota_0 / address=0x00010000\n"
            "BOOT_PARTITION: app0 / ota_0 / address=0x00010000\n"
            "NEXT_UPDATE_PARTITION: app1 / ota_1 / address=0x00150000\n")
        self.known38_ota1 = self.known38.replace(
            "RUNNING_PARTITION: app0 / ota_0 / address=0x00010000",
            "RUNNING_PARTITION: app1 / ota_1 / address=0x00150000"
        ).replace(
            "BOOT_PARTITION: app0 / ota_0 / address=0x00010000",
            "BOOT_PARTITION: app1 / ota_1 / address=0x00150000"
        ).replace(
            "NEXT_UPDATE_PARTITION: app1 / ota_1 / address=0x00150000",
            "NEXT_UPDATE_PARTITION: app0 / ota_0 / address=0x00010000"
        )

    def test_refuse_public_or_loopback_ip(self):
        for ip in ("8.8.8.8", "127.0.0.1", "::1"):
            with self.assertRaises(ValueError):
                b.safe_ip(ip)
        self.assertEqual(b.safe_ip("192.168.0.23"), "192.168.0.23")

    def test_require_exact_slot_order(self):
        f = b.fields(self.known38)
        b.expect(f, "0.0.38", "0", "0")
        with self.assertRaises(RuntimeError):
            b.expect(f, "0.0.38", "1", "1")
        f1 = b.fields(self.known38_ota1)
        b.expect(f1, "0.0.38", "1", "1")

    def test_abort_upload_without_explicit_confirmation(self):
        with mock.patch.object(b, "status", side_effect=AssertionError("must not touch network")):
            with self.assertRaises(RuntimeError):
                b.upload("192.168.0.23", "38", "")

    def test_abort_reboot_without_explicit_confirmation(self):
        with mock.patch.object(b, "status", side_effect=AssertionError("must not touch network")):
            with self.assertRaises(RuntimeError):
                b.reboot("192.168.0.23", "40", "")

    def test_missing_or_wrong_image_aborts(self):
        with self.assertRaises(RuntimeError):
            b.pin_image(self.cache/"missing.bin", "38")
        blob = b"\xe9" + b"a" * 100
        bad = self.cache/"wrong.bin"
        bad.write_bytes(blob)
        with self.assertRaises(RuntimeError):
            b.pin_image(bad, "38")

    def test_pinned_image_is_accepted_only_with_exact_sha_and_size(self):
        payload = b"\xe9" + b"payload"
        m = dict(b.IMAGES["38"])
        m.update(bytes=len(payload),sha=hashlib.sha256(payload).hexdigest())
        path = self.cache/"test.bin"
        path.write_bytes(payload)
        with mock.patch.dict(b.IMAGES, {"38":m}):
            self.assertEqual(b.pin_image(path,"38"),payload)
            path.write_bytes(payload + b"not expected")
            with self.assertRaises(RuntimeError):
                b.pin_image(path,"38")

    def test_upload_refuses_wrong_state_before_flash(self):
        with mock.patch.object(b, "status", return_value=(self.known38_ota1,b.fields(self.known38_ota1))):
            with mock.patch.object(b, "post", side_effect=AssertionError("must not POST")):
                with self.assertRaises(RuntimeError):
                    b.upload("192.168.0.23","38","GRAVAR_038_OTA1")

    def test_reboot_rejects_old_journal(self):
        before = self.known38 + "SOURCE_VERSION: 0.0.37\nOTA_WRITE_VERIFIED: YES\n"
        with mock.patch.object(b,"status",return_value=(before,b.fields(before))):
            with mock.patch.object(b,"post",side_effect=AssertionError("must not POST")):
                with self.assertRaises(RuntimeError):
                    b.reboot("192.168.0.23","38","REINICIAR_038_OTA1")

if __name__ == "__main__":
    unittest.main(verbosity=2)
