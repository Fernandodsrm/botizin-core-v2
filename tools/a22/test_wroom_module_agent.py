#!/usr/bin/env python3
"""Offline tests for A22 authorized module delivery, no LAN hardware touched."""
import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

name="wroom_module_agent"
spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(name+".py"))
a=importlib.util.module_from_spec(spec)
spec.loader.exec_module(a)

JOB={"ID":"led","VERSION":"0.0.32","SIZE":"4","SHA256":"a"*64,
     "BOOT_ID":"0123abcd","EXPIRES_IN":"110"}
MODULE={"id":"led","board":"esp32-wroom-4mb","version":"0.0.32",
        "size":4,"sha256":"a"*64,"return_protocol":2,
        "url":a.RELEASE_PREFIX+"0.0.32/firmware.bin"}

class SafetyTests(unittest.TestCase):
    def test_private_lan_only(self):
        for ip in ["192.168.0.23","10.1.2.3","172.18.0.1"]:
            self.assertTrue(a.trusted_ip(ip))
        for ip in ["127.0.0.1","8.8.8.8","100.105.142.106","2001:db8::1"]:
            self.assertFalse(a.trusted_ip(ip))

    def test_request_requires_physical_window(self):
        with patch.object(a,"get_local",return_value=(204,"")):
            self.assertIsNone(a.pending_request("192.168.0.23"))
        with patch.object(a,"get_local",return_value=(200,
            "BOTIZIN_MODULE_REQUEST_V1\nID: led\nVERSION: 0.0.32\nSIZE: 4\n"
            f"SHA256: {'a'*64}\nBOOT_ID: 0123abcd\nEXPIRES_IN: 1\n")):
            with self.assertRaises(RuntimeError):
                a.pending_request("192.168.0.23")

    def test_firmware_mismatch_rejected(self):
        self.assertEqual(a.verify_selection(JOB,{"led":MODULE}),MODULE)
        with self.assertRaises(RuntimeError):
            a.verify_selection({**JOB,"SHA256":"b"*64},{"led":MODULE})
        with self.assertRaises(RuntimeError):
            a.verify_selection(JOB,{})

    def test_image_cache_requires_matching_hash(self):
        import hashlib
        content=b"test"
        e={**MODULE,"size":len(content),"sha256":hashlib.sha256(content).hexdigest()}
        with tempfile.TemporaryDirectory() as td:
            cache=Path(td)
            (cache/(e["sha256"]+".bin")).write_bytes(content)
            with patch.object(a,"CACHE",cache),patch.object(a.request,"urlopen") as urlopen:
                result,cached=a.load_image(e)
                self.assertTrue(cached)
                self.assertEqual(result,content)
                urlopen.assert_not_called()

    def test_install_refuses_lost_permission(self):
        with patch.object(a,"status_wroom",return_value={"IP":"192.168.0.23"}), \
             patch.object(a,"pending_request",return_value=None), \
             patch.object(a,"post") as post:
            with self.assertRaises(RuntimeError):
                a.install("192.168.0.23",JOB,b"test")
            post.assert_not_called()

    def test_upload_not_triggered_when_catalog_differs(self):
        with patch.object(a,"choose_ip",return_value="192.168.0.23"), \
             patch.object(a,"status_wroom",return_value={}), \
             patch.object(a,"pending_request",return_value=JOB), \
             patch.object(a,"official_catalog",return_value={"led":{**MODULE,"sha256":"b"*64}}), \
             patch.object(a,"load_image") as download:
            with self.assertRaises(RuntimeError):
                a.poll_once()
            download.assert_not_called()

    def test_status_rejects_wrong_slot(self):
        body=("BOTIZIN WROOM V0.0.40\nIP: 192.168.0.23\nWIFI: OK\nOTA: READY\n"
              "RUNNING_PARTITION: app1 / ota_1 / address=0x00150000\n")
        with patch.object(a,"get_local",return_value=(200,body)):
            with self.assertRaises(RuntimeError):
                a.status_wroom("192.168.0.23")

if __name__=="__main__":
    unittest.main()
