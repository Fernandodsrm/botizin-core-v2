#!/usr/bin/env python3
"""One-time fixed-slot WROOM base 0.0.40 installation after full 4MB backup.

Writes *only* OTA0 at 0x10000, preserving bootloader, partitions, otadata,
NVS/Wi-Fi and OTA1. Requires explicit --confirm WRITE_BASE_040.
Not for general ESP32 boards or other serial adapters. Does not publish OTA.
"""
import argparse
import datetime as dt
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

BASE=0x10000
APP_SIZE=0x140000
FLASH=0x400000
KNOWN_WROOM_MAC="30:76:f5:90:67:e0"

def sha(data):
    return hashlib.sha256(data).hexdigest()

def run(args):
    p=subprocess.run(args,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
    print(p.stdout[-5000:])
    if p.returncode:
        raise SystemExit(f"STOP: esptool operation failed: {args[-1]}")
    return p.stdout

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--package",required=True,help="Extracted dist-wroom/0.0.40 folder")
    parser.add_argument("--port",default="COM6")
    parser.add_argument("--backup-dir",required=True,help="Private local folder, never GitHub")
    parser.add_argument("--expected-mac",default=KNOWN_WROOM_MAC)
    parser.add_argument("--confirm",default="",help="Must equal WRITE_BASE_040 to write")
    a=parser.parse_args()
    root=Path(a.package).resolve()
    artifact=root/"firmware.ino.bin"
    binfile=artifact.read_bytes()
    record=json.loads((root/"build-record.json").read_text(encoding="utf-8"))
    checks=json.loads((root/"checksums.json").read_text(encoding="utf-8"))
    if not (record.get("version")=="0.0.40" and record.get("partitions_verified") is True
            and record.get("credentials_compiled") is False
            and record.get("published") is False
            and 0<len(binfile)<=APP_SIZE
            and record.get("bytes")==len(binfile)
            and record.get("sha256")==sha(binfile)
            and checks.get("firmware.ino.bin")==sha(binfile)
            and binfile[0]==0xe9
            and b"BOTIZIN WROOM V" in binfile
            and b"0.0.40" in binfile):
        raise SystemExit("STOP: invalid, unexpected or unverified WROOM 0.0.40 package")
    # Confirm this is only for the correct physical device and 4 MB layout.
    expected=a.expected_mac.lower()
    if not re.fullmatch(r"(?:[0-9a-f]{2}:){5}[0-9a-f]{2}",expected):
        raise SystemExit("STOP: invalid expected MAC")
    command=[sys.executable,"-m","esptool","--chip","esp32","--port",a.port]
    info=run(command+["flash-id"])
    if (expected not in info.lower() or
        not any(x in info.lower() for x in ["detected flash size: 4mb","flash size: 4mb"])):
        raise SystemExit("STOP: WROOM identity/flash differs; no write")
    backup_root=Path(a.backup_dir).resolve()
    if backup_root == root or root in backup_root.parents or "botizin-core-v2" in str(backup_root).lower():
        raise SystemExit("STOP: backup must stay outside GitHub/repository")
    backup_root.mkdir(mode=0o700,parents=True,exist_ok=True)
    stamp=dt.datetime.now(dt.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    backup=backup_root/f"wroom-4mb-before-040-{stamp}.bin"
    print("READING full 4MB recovery backup to",backup)
    run(command+["read-flash","0x0","0x400000",str(backup)])
    data=backup.read_bytes()
    if len(data)!=FLASH or b"BOTIZIN WROOM V" not in data:
        raise SystemExit("STOP: backup invalid or not WROOM; no write")
    checksum=sha(data)
    (backup_root/(backup.name+".sha256")).write_text(checksum+"  "+backup.name+"\n")
    print("BACKUP VERIFIED 4194304 bytes; SHA256",checksum)
    print("NEW BASE: WROOM 0.0.40; OTA0 only 0x10000; partitions/NVS/OTA1 unchanged")
    if a.confirm!="WRITE_BASE_040":
        print("DRY RUN: no flash written. Re-run with --confirm WRITE_BASE_040 after approval.")
        return
    print("WRITING ONLY OTA0 firmware (not partition table or NVS)...")
    run(command+["write-flash","--flash-mode","dio","--flash-size","4MB",
                 "0x10000",str(artifact)])
    print("WRITE COMPLETED. Reboot and verify GET /status VERSION 0.0.40, OTA READY, RUNNING ota_0.")
    print("DO NOT discard the recovery backup until module -> BOTIZIN return is physically proven.")

if __name__=="__main__":
    main()
