#!/usr/bin/env python3
"""BOTIZIN WROOM 0.0.40 module loader; A22 runs it, WROOM physically approves.

Never flashes the WROOM base (app0), edits partitions or exposes credentials.
Only accepts a pending 120s physical-button request from WROOM v0.0.40,
compares against the official HTTPS GitHub catalog and sha256, and POSTs the
exact release image into WROOM's existing local firmware upload endpoint.
Use --once for a supervised poll, --watch for a low-rate long-running service.
"""
import argparse
import fcntl
import datetime as dt
import hashlib
import http.client
import ipaddress
import json
import os
from pathlib import Path
import re
import tempfile
import time
from urllib import error, parse, request

MAP=Path("/root/botizin-mcp/dispositivos.json")
CACHE=Path("/root/botizin-mcp/wroom-module-cache")
CATALOG="https://raw.githubusercontent.com/Fernandodsrm/botizin-core-v2/main/wroom/catalog.json"
RELEASE_PREFIX="https://raw.githubusercontent.com/Fernandodsrm/botizin-core-v2/main/wroom/releases/"
MAX_MODULE=0x140000
MAX_CATALOG=8192
IMAGE_TIMEOUT=35
TRUSTED_NETWORKS=tuple(ipaddress.ip_network(x) for x in ("10.0.0.0/8","172.16.0.0/12","192.168.0.0/16"))
LOCAL_OPENER=request.build_opener(request.ProxyHandler({}))


def log(event, **values):
    row={"at":dt.datetime.now(dt.timezone.utc).isoformat(timespec="seconds"),"event":event}
    row.update(values)
    print(json.dumps(row,sort_keys=True,ensure_ascii=False),flush=True)


def trusted_ip(ip):
    try:
        addr=ipaddress.ip_address(ip)
        return isinstance(addr,ipaddress.IPv4Address) and any(addr in net for net in TRUSTED_NETWORKS)
    except ValueError:
        return False


def choose_ip():
    try:
        state=json.loads(MAP.read_text(encoding="utf-8"))
        record=state["dispositivos"]["wroom"]
        for key in ("ip","last_known_ip"):
            ip=record.get(key)
            if isinstance(ip,str) and trusted_ip(ip):
                return ip
    except (OSError,TypeError,ValueError,KeyError):
        pass
    return None


def get_local(ip,path,timeout=3):
    if not trusted_ip(ip):
        raise ValueError("unsafe local address")
    with LOCAL_OPENER.open(f"http://{ip}{path}",timeout=timeout) as resp:
        return resp.status, resp.read(10000).decode("utf-8","strict")


def status_wroom(ip):
    code, body=get_local(ip,"/status")
    if code!=200 or not re.match(r"^BOTIZIN WROOM V0\.0\.(?:40|41)\n",body):
        raise RuntimeError("not WROOM base 0.0.40")
    fields=parse_fields(body)
    if fields.get("IP")!=ip or fields.get("WIFI")!="OK" or fields.get("OTA")!="READY":
        raise RuntimeError("wrong WROOM identity/connection")
    if not fields.get("RUNNING_PARTITION","").startswith("app0 / ota_0"):
        raise RuntimeError("base must execute in ota0; will not modify")
    return fields


def parse_fields(body):
    result={}
    for line in body.splitlines():
        match=re.fullmatch(r"([A-Z0-9_]+):[ \t]*(.{0,180})",line)
        if match:
            result[match.group(1)]=match.group(2)
    return result


def pending_request(ip):
    status, body=get_local(ip,"/module/request")
    if status==204:
        return None
    if status!=200 or not body.startswith("BOTIZIN_MODULE_REQUEST_V1\n"):
        raise RuntimeError("invalid module request")
    data=parse_fields(body)
    required=("ID","VERSION","SIZE","SHA256","BOOT_ID","EXPIRES_IN")
    if any(x not in data for x in required):
        raise RuntimeError("missing module request fields")
    if not re.fullmatch(r"[a-z0-9][a-z0-9_-]{0,23}",data["ID"]):
        raise RuntimeError("invalid module id")
    if not re.fullmatch(r"0\.0\.\d{1,3}",data["VERSION"]):
        raise RuntimeError("invalid module version")
    if not re.fullmatch(r"[a-f0-9]{64}",data["SHA256"]):
        raise RuntimeError("invalid module sha")
    if not re.fullmatch(r"[a-f0-9]{1,32}",data["BOOT_ID"]):
        raise RuntimeError("invalid boot_id")
    if not data["SIZE"].isdigit() or not 0<int(data["SIZE"])<=MAX_MODULE:
        raise RuntimeError("invalid module size")
    if not data["EXPIRES_IN"].isdigit() or not 5<=int(data["EXPIRES_IN"])<=120:
        raise RuntimeError("physical authorization expired")
    return data


def official_catalog():
    with request.urlopen(CATALOG,timeout=12) as resp:
        if resp.status!=200:
            raise RuntimeError("GitHub catalog not ready")
        raw=resp.read(MAX_CATALOG+1)
    if len(raw)>MAX_CATALOG:
        raise RuntimeError("GitHub catalog too large")
    data=json.loads(raw)
    if data.get("schema")!=1 or not isinstance(data.get("environments"),list):
        raise RuntimeError("unsupported GitHub catalog")
    environments={}
    for e in data["environments"]:
        if not isinstance(e,dict) or e.get("board")!="esp32-wroom-4mb":
            raise RuntimeError("invalid module board")
        ident=e.get("id")
        if not isinstance(ident,str) or not re.fullmatch(r"[a-z0-9][a-z0-9_-]{0,23}",ident) or ident in environments:
            raise RuntimeError("invalid or duplicate catalog id")
        url=e.get("url","")
        size=e.get("size",0)
        sha=e.get("sha256","")
        if (not isinstance(url,str) or not url.startswith(RELEASE_PREFIX)
                or not url.endswith("/firmware.bin")
                or not isinstance(size,int) or isinstance(size,bool) or not 0<size<=MAX_MODULE
                or not isinstance(sha,str) or not re.fullmatch(r"[a-f0-9]{64}",sha)
                or e.get("return_protocol")!=2):
            raise RuntimeError("invalid trusted module metadata")
        environments[ident]=e
    return environments


def verify_selection(job,catalog):
    e=catalog.get(job["ID"])
    if e is None or (e["version"]!=job["VERSION"] or
                     str(e["size"])!=job["SIZE"] or
                     e["sha256"]!=job["SHA256"]):
        raise RuntimeError("request differs from official catalog or WROOM pinned record")
    return e


def load_image(e):
    CACHE.mkdir(mode=0o700,parents=True,exist_ok=True)
    os.chmod(CACHE,0o700)
    target=CACHE/(e["sha256"]+".bin")
    if target.is_file():
        contents=target.read_bytes()
        if len(contents)==e["size"] and hashlib.sha256(contents).hexdigest()==e["sha256"]:
            return contents,True
    with request.urlopen(e["url"],timeout=IMAGE_TIMEOUT) as resp:
        if resp.status!=200:
            raise RuntimeError("module download failed")
        contents=resp.read(e["size"]+1)
    if len(contents)!=e["size"] or hashlib.sha256(contents).hexdigest()!=e["sha256"]:
        raise RuntimeError("module hash/length mismatch")
    tmp=None
    try:
        with tempfile.NamedTemporaryFile("wb",dir=CACHE,prefix=".incoming-",delete=False) as fp:
            tmp=fp.name
            os.chmod(tmp,0o600)
            fp.write(contents)
            fp.flush();os.fsync(fp.fileno())
        os.replace(tmp,target)
    finally:
        if tmp and os.path.exists(tmp):
            os.unlink(tmp)
    return contents,False


def post(ip,path,body,ctype,timeout=35):
    if not trusted_ip(ip):
        raise ValueError("invalid WROOM IP")
    connection=http.client.HTTPConnection(ip,80,timeout=timeout)
    try:
        connection.request("POST",path,body=body,headers={
            "Content-Type":ctype,
            "Connection":"close"
        })
        response=connection.getresponse()
        data=response.read(9000).decode("utf-8","replace")
        return response.status,data
    finally:
        connection.close()


def install(ip,job,image):
    # Re-check physical authorization after network download and before upload.
    fields=status_wroom(ip)
    fresh=pending_request(ip)
    if fresh is None or any(fresh[x]!=job[x] for x in ("ID","VERSION","SIZE","SHA256","BOOT_ID")):
        raise RuntimeError("user approval changed/expired")
    boundary="botizin-module-"+job["BOOT_ID"]
    prefix=(f"--{boundary}\r\n"
            'Content-Disposition: form-data; name="firmware"; filename="module.bin"\r\n'
            "Content-Type: application/octet-stream\r\n\r\n").encode()
    suffix=f"\r\n--{boundary}--\r\n".encode()
    path=f'/update?size={job["SIZE"]}&sha256={job["SHA256"]}'
    status,body=post(ip,path,prefix+image+suffix,f"multipart/form-data; boundary={boundary}",timeout=55)
    if status!=200 or "OTA_WRITE_VERIFIED: YES" not in body:
        raise RuntimeError(f"module upload not verified; HTTP {status}")
    # Trigger boot only after the firmware itself verified the image and anchor.
    status,reboot=post(ip,"/reboot",b"","application/octet-stream",timeout=8)
    if status!=200 or "Reboot in" not in reboot:
        raise RuntimeError(f"module verified but reboot not confirmed; HTTP {status}")
    log("module_boot_requested",module=job["ID"],version=job["VERSION"],ip=ip)


def poll_once():
    ip=choose_ip()
    if ip is None:
        return False
    status_wroom(ip)
    job=pending_request(ip)
    if job is None:
        return False
    catalog=official_catalog()
    selection=verify_selection(job,catalog)
    binary,cached=load_image(selection)
    log("module_verified",module=job["ID"],bytes=len(binary),cached=cached,ip=ip)
    install(ip,job,binary)
    return True


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--watch",action="store_true",help="poll every 5 seconds")
    args=parser.parse_args()
    # A supervisor restart must not create two competing module uploads.
    lockpath=Path("/root/botizin-mcp/.wroom-module-agent.lock")
    lockpath.parent.mkdir(parents=True,exist_ok=True)
    fd=os.open(lockpath,os.O_CREAT|os.O_RDWR,0o600)
    try:
        with os.fdopen(fd,"w") as lock:
            try:fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
            except BlockingIOError:return
            return run_authorized_agent(args)
    except BaseException:
        raise

def run_authorized_agent(args):
    if args.watch:
        while True:
            try:
                poll_once()
            except Exception as exc:
                # Errors are reported but cannot start another firmware operation.
                log("module_poll_error",reason=type(exc).__name__,details=str(exc)[:180])
            time.sleep(5)
    else:
        try:
            poll_once()
        except Exception as exc:
            log("module_poll_error",reason=type(exc).__name__,details=str(exc)[:180])
            raise SystemExit(2)


if __name__=="__main__":
    main()
