#!/usr/bin/env python3
"""Offline safety tests: never connect to a real ESP32 in CI."""
import binascii
import pathlib
import struct
import sys
import unittest
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[0]))
from install_stable import patch_ota, ota_entries, parse_partition_table, APP0, APP1, APP_SIZE

def apply_ota(original,target):
    off,sector,seq=patch_ota(original,target)
    b=bytearray(original)
    pos=off-0xe000
    b[pos:pos+4096]=sector
    return bytes(b),seq

class TestSelector(unittest.TestCase):
    def test_initial_and_return(self):
        raw=bytes([255])*8192
        slot1,seq1=apply_ota(raw,1)
        self.assertEqual(seq1,2)
        self.assertEqual(ota_entries(slot1)[0],(2,2,True))
        slot0,seq0=apply_ota(slot1,0)
        self.assertEqual(seq0,3)
        self.assertEqual(ota_entries(slot0)[1],(3,2,True))
        self.assertEqual(slot0[:4096],slot1[:4096])
        self.assertEqual(binascii.crc32(struct.pack("<I",seq1),0xffffffff)&0xffffffff,
                         ota_entries(slot1)[0][0] and struct.unpack_from("<I",slot1,28)[0])

    def test_existing_active_base(self):
        raw,seq=apply_ota(bytes([255])*8192,0)
        self.assertEqual(seq,1)
        pilot,seq2=apply_ota(raw,1)
        self.assertEqual(seq2,2)
        self.assertEqual(ota_entries(pilot)[0],(1,2,True))
        self.assertEqual(ota_entries(pilot)[1],(2,2,True))

    def test_bad_metadata(self):
        with self.assertRaises(SystemExit):
            patch_ota(b"garbage",1)
        with self.assertRaises(SystemExit):
            patch_ota(bytes([255])*8192,3)

    def test_layout(self):
        image=bytearray(b"\xff"*0x400000)
        for i,(t,s,off,size) in enumerate(((0,16,APP0,APP_SIZE),(0,17,APP1,APP_SIZE),(1,0,0xe000,0x2000))):
            struct.pack_into("<HBBII",image,0x8000+i*32,0x50aa,t,s,off,size)
        image[APP0:APP0+24]=b"BOTIZIN WROOM V0.0.40"
        parse_partition_table(bytes(image))
        struct.pack_into("<I",image,0x8000+32+4,0x160000)
        with self.assertRaises(SystemExit):
            parse_partition_table(bytes(image))

if __name__=="__main__":
    unittest.main()
