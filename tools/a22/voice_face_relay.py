#!/usr/bin/env python3
"""A22 -> WROOM OLED relay of S3/XiaoZhi *state only* (no sound/transcript).
Run --once to prove the link; --watch only after WROOM LAB has been tested.
No credentials, OTA, NVS, file writes or shell execution.
"""
import argparse
import http.client
import ipaddress
import json
import time

S3_IP = "192.168.0.24"
WROOM_IP = "192.168.0.23"
ALLOWED = (3, 4, 5, 6)


def private_ip(address):
    try:
        a = ipaddress.ip_address(address)
        return isinstance(a, ipaddress.IPv4Address) and a.is_private and not a.is_loopback
    except ValueError:
        return False


def fetch(ip, path, timeout=0.8):
    if not private_ip(ip):
        raise ValueError("network address outside private IPv4")
    conn = http.client.HTTPConnection(ip, 80, timeout=timeout)
    try:
        conn.request("GET", path)
        response = conn.getresponse()
        body = response.read(8193)
        if response.status != 200 or len(body) > 8192:
            raise RuntimeError("Unexpected HTTP status/size")
        return body
    finally:
        conn.close()


def parse_s3(body):
    data = json.loads(body.decode("utf-8"))
    if data.get("origin") != "XIAOZHI_REAL":
        raise ValueError("Wrong S3 identity")
    state = data.get("state")
    if type(state) is not int:
        raise ValueError("Invalid state type")
    if state == 9:
        state = 4
    if state not in ALLOWED:
        raise ValueError("Unrecognized state")
    if data.get("web_audio_connected") is not True:
        # With no active mic/speaker the OLED should remain calm.
        return 3
    return state


def check_wroom(body, ip):
    text = body.decode("utf-8")
    return (
        text.startswith("BOTIZIN WROOM V0.0.41\n")
        and f"IP: {ip}\n" in text
        and ("RUNNING_PARTITION: app0 / ota_0" in text
             or "RUNNING_PARTITION: app1 / ota_1" in text)
        and "WIFI: OK\n" in text
        and "VOICE_SYNC_MODE:" in text
    )


def post_voice(ip, state):
    if not private_ip(ip) or state not in ALLOWED:
        raise ValueError("Invalid destination/state")
    conn = http.client.HTTPConnection(ip, 80, timeout=0.8)
    try:
        conn.request("POST", f"/face/voice?state={state}", body=b"",
                     headers={"Content-Length": "0"})
        r = conn.getresponse()
        body = r.read(100)
        if r.status != 200 or body != b"VOICE_SYNC_OK":
            raise RuntimeError(f"WROOM voice endpoint status {r.status}: {body!r}")
    finally:
        conn.close()


def run(once=False, dry_run=False):
    if not dry_run and not check_wroom(fetch(WROOM_IP, "/status"), WROOM_IP):
        raise RuntimeError("WROOM .41 base not confirmed. Relay will not start.")
    last = None
    sent_at = 0.0
    checked_at = time.monotonic()
    while True:
        start = time.monotonic()
        try:
            if not dry_run and start - checked_at >= 30:
                if not check_wroom(fetch(WROOM_IP, "/status"), WROOM_IP):
                    raise RuntimeError("WROOM identity changed")
                checked_at = start
            mode = parse_s3(fetch(S3_IP, "/status"))
            if mode != last or start - sent_at >= 1.8:
                if not dry_run:
                    post_voice(WROOM_IP, mode)
                print(f"S3 state={mode} -> WROOM {('DRY_RUN' if dry_run else 'VOICE_SYNC_OK')}", flush=True)
                last = mode
                sent_at = start
        except Exception as exc:
            # On network failure do not repeatedly push an old state:
            # WROOM's 4-second TTL automatically restores the local face.
            print(f"Voice relay unavailable: {type(exc).__name__}: {exc}", flush=True)
            if once:
                return 1
            time.sleep(1.5)
            continue
        if once:
            return 0
        time.sleep(max(0, 0.8-(time.monotonic()-start)))


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    mode = ap.add_mutually_exclusive_group(required=True)
    mode.add_argument("--once", action="store_true")
    mode.add_argument("--watch", action="store_true")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()
    try:
        raise SystemExit(run(once=args.once, dry_run=args.dry_run))
    except KeyboardInterrupt:
        raise SystemExit(0)
