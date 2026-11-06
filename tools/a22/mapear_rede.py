#!/usr/bin/env python3
"""Read-only LAN discovery and persistent device map for BOTIZIN."""
import concurrent.futures
import datetime
import ipaddress
import json
import os
from pathlib import Path
import re
import socket
import struct
import tempfile
import urllib.request

STATE = Path("/root/botizin-mcp/dispositivos.json")
NETWORK = ipaddress.ip_network("192.168.0.0/24")
DEVICES = {
    "s3": ("botizin-s3.local", "BOTIZIN CORE V"),
    "wroom": ("botizin-wroom.local", "BOTIZIN WROOM V"),
}
def default_gateway():
    try:
        with open("/proc/net/route") as f:
            next(f)
            for line in f:
                a=line.split()
                if len(a)>=4 and a[1]=="00000000" and int(a[3],16)&2:
                    ip=socket.inet_ntoa(struct.pack("<L",int(a[2],16)))
                    return ip
    except (OSError,ValueError,IndexError):
        pass
    return None

def verify(ip, timeout=1.4):
    try:
        addr=ipaddress.ip_address(ip)
        if addr not in NETWORK:return None
        with urllib.request.urlopen(f"http://{addr}/status",timeout=timeout) as r:
            if r.status!=200:return None
            body=r.read(6000).decode("utf-8","replace")
        lines=body.splitlines()
        if not lines:return None
        kind=next((k for k,(_,signature) in DEVICES.items() if lines[0].startswith(signature)),None)
        if kind is None:return None
        data=dict(re.findall(r"(?m)^([A-Z_]+):[ \t]*([^\r\n]+)$",body))
        advertised=data.get("IP","").strip()
        if advertised!=str(addr):
            return None  # IP/identity mismatch, do not trust as this device
        return {"kind":kind,"ip":str(addr),"name":DEVICES[kind][0],
                "firmware":lines[0].strip(),"wifi":data.get("WIFI","?"),
                "boot_id":data.get("BOOT_ID"),"led_state":data.get("LED_STATE"),
                "ota":data.get("OTA")}
    except (OSError,ValueError,TimeoutError,UnicodeError):
        return None

def load_saved():
    try:
        d=json.loads(STATE.read_text())
        return d.get("dispositivos",{})
    except (OSError,ValueError,TypeError):
        return {}

def lookup_mdns(host):
    try:
        for info in socket.getaddrinfo(host,80,family=socket.AF_INET,type=socket.SOCK_STREAM):
            addr=info[4][0]
            if ipaddress.ip_address(addr) in NETWORK:return addr
    except (OSError,ValueError):pass
    return None

def discover():
    saved=load_saved()
    known={}
    candidates=set()
    for kind,(name,_) in DEVICES.items():
        ip=saved.get(kind,{}).get("ip")
        if ip:candidates.add(ip)
        mdns=lookup_mdns(name)
        if mdns:candidates.add(mdns)
    # Fast path: previously seen and mDNS names. Every response must self-identify.
    with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:
        for d in pool.map(verify,candidates):
            if d:known[d["kind"]]=d
    if len(known)!=len(DEVICES):
        # Only scan the known private home LAN, not VPN or public networks.
        with concurrent.futures.ThreadPoolExecutor(max_workers=24) as pool:
            for d in pool.map(verify,(str(x) for x in NETWORK.hosts())):
                if d:known[d["kind"]]=d
    now=datetime.datetime.now(datetime.timezone.utc).isoformat()
    mapped={}
    for kind,(name,_) in DEVICES.items():
        if kind in known:
            mapped[kind]={**known[kind],"online":True,"checked_at_utc":now}
        else:
            prev=saved.get(kind,{})
            mapped[kind]={"kind":kind,"name":name,"online":False,
                          "last_known_ip":prev.get("ip") or prev.get("last_known_ip"),
                          "checked_at_utc":now}
    return {"schema":1,"checked_at_utc":now,"gateway":default_gateway(),
            "subnet":str(NETWORK),"dispositivos":mapped}

def main():
    result=discover()
    STATE.parent.mkdir(parents=True,exist_ok=True)
    handle=tempfile.NamedTemporaryFile("w",dir=STATE.parent,prefix=".botizin-map-",delete=False)
    try:
        os.chmod(handle.name,0o600)
        json.dump(result,handle,ensure_ascii=False,indent=2)
        handle.write("\n")
        handle.flush()
        os.fsync(handle.fileno())
        handle.close()
        os.replace(handle.name,STATE)
    finally:
        if not handle.closed:handle.close()
        if os.path.exists(handle.name):os.unlink(handle.name)
    for name,d in result["dispositivos"].items():
        print(f"{name}: "+(d["ip"] if d["online"] else "offline"))
    print("saved:",str(STATE))
if __name__=="__main__":main()
