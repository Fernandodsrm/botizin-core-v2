#!/usr/bin/env python3
"""Offline parser and identity tests: no LAN calls in CI."""
import unittest
from voice_face_relay import parse_s3, check_wroom, private_ip

class VoiceRelayTest(unittest.TestCase):
    def test_s3_identity_and_states(self):
        for state in (3,4,5,6,9):
            raw=('{"origin":"XIAOZHI_REAL","state":%d,"web_audio_connected":true}' % state).encode()
            self.assertEqual(parse_s3(raw),4 if state==9 else state)
        self.assertEqual(parse_s3(b'{"origin":"XIAOZHI_REAL","state":6,"web_audio_connected":false}'),3)
        for value in (b'{"state":5}',b'{"origin":"XIAOZHI_REAL","state":2}',b'{"origin":"XIAOZHI_REAL","state":"5"}'):
            with self.assertRaises(ValueError):
                parse_s3(value)

    def test_wroom_identity(self):
        status=b"BOTIZIN WROOM V0.0.41\nIP: 192.168.0.23\nRUNNING_PARTITION: app0 / ota_0 / address=0x00010000\nWIFI: OK\nVOICE_SYNC_MODE: 5\n"
        self.assertTrue(check_wroom(status,"192.168.0.23"))
        self.assertFalse(check_wroom(status.replace(b"0.0.41",b"0.0.40"),"192.168.0.23"))
        self.assertFalse(check_wroom(status,"192.168.0.66"))

    def test_private_addresses(self):
        self.assertTrue(private_ip("192.168.0.23"))
        for ip in ("127.0.0.1","8.8.8.8","not-an-ip","2001:db8::1"):
            self.assertFalse(private_ip(ip))

if __name__=="__main__":
    unittest.main()
