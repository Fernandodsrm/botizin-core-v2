#!/usr/bin/env python3
"""BOTIZIN A22: migrate WROOM 0.0.38 OTA0 -> 0.0.38 OTA1 -> 0.0.40 OTA0.

No USB, no repartition and no changes to the main OTA manifest.
Default 'inspect' is read-only. Prepare downloads only; writes and reboots
are distinct explicit commands with confirmation strings.

IMPORTANT: No firmware-only method can GUARANTEE recovery if new OTA0 fails
before networking works. Keep USB recovery as the last resort.
"""
import argparse
import hashlib
import http.client
import ipaddress
import json
import os
from pathlib import Path
import socket
import sys
import tempfile
import time
from urllib.parse import urlencode
from urllib.request import urlopen

FLASH_SLOT = 0x140000
CACHE = Path("/root/botizin-mcp/ota-migration-cache")
DEVICE_MAP = Path("/root/botizin-mcp/dispositivos.json")
RAW38 = ("https://raw.githubusercontent.com/Fernandodsrm/botizin-core-v2/"
         "main/wroom/releases/0.0.38/firmware.bin")
RAW40 = ("https://github.com/Fernandodsrm/botizin-core-v2/releases/download/"
         "wroom-040-ota-lab-1a52d586/firmware.bin")
IMAGES = {
    "38": {"version": "0.0.38", "bytes": 1288144,
           "sha": "27739579d6e8dbeb1608b9f0e1200919fd90d1c563aeb4ab1848fc27d629fd13",
           "url": RAW38},
    "40": {"version": "0.0.40", "bytes": 1069744,
           "sha": "1a52d58619bdfa8fb7af505c40fc668dc35f233b184ced58f50be41bbe28ef51",
           "url": RAW40},
}
SLOT = {"0": "app0 / ota_0", "1": "app1 / ota_1"}


def safe_ip(value):
    ip = ipaddress.ip_address(value)
    if not isinstance(ip, ipaddress.IPv4Address) or not ip.is_private or ip.is_loopback:
        raise ValueError("refused: address must be a private IPv4 LAN host")
    return str(ip)


def get_ip(explicit):
    if explicit:
        return safe_ip(explicit)
    data = json.loads(DEVICE_MAP.read_text(encoding="utf-8"))
    rec = data["dispositivos"]["wroom"]
    return safe_ip(rec.get("ip") or rec["last_known_ip"])


def fields(body):
    result = {}
    for line in body.splitlines():
        if ": " in line:
            key, value = line.split(": ", 1)
            result.setdefault(key, value)
    return result


def status(ip):
    conn = http.client.HTTPConnection(ip, 80, timeout=8)
    try:
        conn.request("GET", "/status")
        resp = conn.getresponse()
        body = resp.read(16000).decode("utf-8", "replace")
        if resp.status != 200 or not body.startswith("BOTIZIN WROOM V"):
            raise RuntimeError("device did not identify as BOTIZIN WROOM")
        f = fields(body)
        if f.get("FLASH_BYTES") != "4194304" or f.get("OTA") != "READY":
            raise RuntimeError("flash geometry or OTA readiness mismatch")
        return body, f
    finally:
        conn.close()


def expect(f, version, running, boot):
    pairs = {
        "RUNNING_PARTITION": SLOT[running],
        "BOOT_PARTITION": SLOT[boot],
    }
    for key, start in pairs.items():
        if not f.get(key, "").startswith(start):
            raise RuntimeError("STOP: " + key + " mismatch; got " + str(f.get(key)))
    if f.get("NEXT_UPDATE_PARTITION", "").startswith(SLOT[running]):
        raise RuntimeError("STOP: OTA destination equals running slot")
    if f.get("OTA") != "READY":
        raise RuntimeError("STOP: OTA unavailable")


def pin_image(path, version):
    meta = IMAGES[version]
    if not path.is_file() or path.stat().st_size != meta["bytes"]:
        raise RuntimeError("STOP: size mismatch or missing cached image " + str(path))
    blob = path.read_bytes()
    if len(blob) > FLASH_SLOT or not blob.startswith(b"\xe9"):
        raise RuntimeError("STOP: not a valid-sized ESP32 image")
    digest = hashlib.sha256(blob).hexdigest()
    if digest != meta["sha"]:
        raise RuntimeError("STOP: SHA256 image mismatch")
    return blob


def prepare(version):
    meta = IMAGES[version]
    CACHE.mkdir(mode=0o700, parents=True, exist_ok=True)
    path = CACHE / ("wroom-" + meta["version"] + ".bin")
    if path.exists():
        pin_image(path, version)
        print("CACHED_VERIFIED", path)
        return
    with urlopen(meta["url"], timeout=35) as r:
        data = r.read(FLASH_SLOT + 1)
    if len(data) != meta["bytes"] or hashlib.sha256(data).hexdigest() != meta["sha"]:
        raise RuntimeError("STOP: official image hash/size differ; cache not touched")
    fd, name = tempfile.mkstemp(prefix=".candidate-", dir=CACHE)
    try:
        with os.fdopen(fd, "wb") as fh:
            fh.write(data)
        os.chmod(name, 0o600)
        os.replace(name, path)
    finally:
        if os.path.exists(name):
            os.unlink(name)
    pin_image(path, version)
    print("DOWNLOADED_AND_VERIFIED", path, "bytes", meta["bytes"], "sha256", meta["sha"])


def post(ip, route, blob=None, version=None):
    conn = http.client.HTTPConnection(ip, 80, timeout=45)
    try:
        if blob is None:
            conn.request("POST", route, body=b"")
        else:
            assert version in IMAGES
            m = IMAGES[version]
            boundary = "----BOTIZIN-OTA-EXPLICIT-1"
            head = ('--' + boundary + '\r\nContent-Disposition: form-data; '
                    'name="firmware"; filename="firmware.bin"\r\n'
                    'Content-Type: application/octet-stream\r\n\r\n').encode("ascii")
            tail = ('\r\n--' + boundary + '--\r\n').encode("ascii")
            query = urlencode({"size": m["bytes"], "sha256": m["sha"]})
            conn.putrequest("POST", route + "?" + query)
            conn.putheader("Content-Type", "multipart/form-data; boundary=" + boundary)
            conn.putheader("Content-Length", str(len(head) + len(blob) + len(tail)))
            conn.endheaders()
            conn.send(head)
            for pos in range(0, len(blob), 8192):
                conn.send(blob[pos:pos + 8192])
            conn.send(tail)
        res = conn.getresponse()
        data = res.read(16000).decode("utf-8", "replace")
        if res.status != 200:
            raise RuntimeError("STOP: device rejected HTTP " + str(res.status) + ": " + data[:500])
        return data
    finally:
        conn.close()


def inspect(ip):
    body, f = status(ip)
    print(body.splitlines()[0])
    for key in ("IP", "RUNNING_PARTITION", "BOOT_PARTITION",
                "NEXT_UPDATE_PARTITION", "OTA", "OTA_AUTOMATIC_ENABLED"):
        print(key, f.get(key, "UNKNOWN"))
    return f


def upload(ip, version, confirm):
    if version == "38":
        token, running, next_boot = "GRAVAR_038_OTA1", "0", "1"
    else:
        token, running, next_boot = "GRAVAR_040_OTA0", "1", "0"
    if confirm != token:
        raise RuntimeError("STOP: explicit --confirm " + token + " required")
    body, f = status(ip)
    if not body.startswith("BOTIZIN WROOM V0.0.38\n"):
        raise RuntimeError("STOP: OTA migration must be transmitted by WROOM v0.0.38")
    expect(f, "0.0.38", running, running)
    if not f.get("NEXT_UPDATE_PARTITION", "").startswith(SLOT[next_boot]):
        raise RuntimeError("STOP: wrong target partition")
    if f.get("OTA_AUTOMATIC_ENABLED") != "NO":
        raise RuntimeError("STOP: disable automatic OTA before migration")
    path = CACHE / ("wroom-" + IMAGES[version]["version"] + ".bin")
    blob = pin_image(path, version)
    print("VALIDATED:", "from OTA" + running, "to OTA" + next_boot, "sha256", IMAGES[version]["sha"])
    answer = post(ip, "/update", blob, version)
    if "OTA_WRITE_VERIFIED: YES" not in answer:
        raise RuntimeError("STOP: POST returned without verified-write evidence")
    after, af = status(ip)
    expect(af, "0.0.38", running, next_boot)
    if ("SOURCE_VERSION: 0.0.38" not in after or
            "SHA_EXPECTED: " + IMAGES[version]["sha"] not in after):
        raise RuntimeError("STOP: journal does not match newly uploaded image")
    print("FLASH_VERIFIED_BOOT_PENDING", "OTA" + next_boot)
    print("NOT_REBOOTED. Run reboot" + version + " with explicit confirmation.")


def reboot(ip, version, confirm):
    if version == "38":
        token, running, next_boot, new_version = "REINICIAR_038_OTA1", "0", "1", "0.0.38"
    else:
        token, running, next_boot, new_version = "REINICIAR_040_OTA0", "1", "0", "0.0.40"
    if confirm != token:
        raise RuntimeError("STOP: explicit --confirm " + token + " required")
    before, f = status(ip)
    if not before.startswith("BOTIZIN WROOM V0.0.38\n"):
        raise RuntimeError("STOP: source is not WROOM 0.0.38")
    expect(f, "0.0.38", running, next_boot)
    if ("SHA_EXPECTED: " + IMAGES[version]["sha"] not in before or
            "SOURCE_VERSION: 0.0.38" not in before or
            "OTA_WRITE_VERIFIED: YES" not in before):
        raise RuntimeError("STOP: journal does not prove expected image")
    print("REBOOT_REQUESTED", "target OTA" + next_boot, flush=True)
    print(post(ip, "/reboot")[:300], flush=True)
    for _ in range(45):
        time.sleep(2)
        try:
            now, nf = status(ip)
            if now.startswith("BOTIZIN WROOM V" + new_version + "\n"):
                expect(nf, new_version, next_boot, next_boot)
                print("BOOT_SUCCESS", new_version, "OTA" + next_boot)
                return
        except (OSError, RuntimeError, socket.timeout):
            pass
    raise RuntimeError("STOP: target firmware did not confirm over LAN. "
                       "Do not retry OTA blindly; physical USB recovery may be needed.")


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("action", choices=("inspect", "prepare38", "upload38", "reboot38",
                                      "prepare40", "upload40", "reboot40"))
    p.add_argument("--ip", help="WROOM local private IPv4; default uses A22 devices map")
    p.add_argument("--confirm", default="")
    a = p.parse_args()
    if a.action.startswith("prepare"):
        prepare(a.action[-2:])
        return
    ip = get_ip(a.ip)
    if a.action == "inspect":
        inspect(ip)
    elif a.action.startswith("upload"):
        upload(ip, a.action[-2:], a.confirm)
    else:
        reboot(ip, a.action[-2:], a.confirm)


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError, json.JSONDecodeError, RuntimeError) as error:
        print("STOP:", error, file=sys.stderr)
        sys.exit(2)
