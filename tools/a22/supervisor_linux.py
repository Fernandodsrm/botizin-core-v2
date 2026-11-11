#!/usr/bin/env python3
"""BOTIZIN Linux supervisor v2. Ordered, idempotent, health-based recovery."""
import fcntl
import json
import os
from pathlib import Path
import signal
import socket
import subprocess
import tempfile
import time
import urllib.error
import urllib.request

ROOT=Path("/root/botizin-mcp")
PYTHON=ROOT/".venv/bin/python"
SERVER=ROOT/"servidor.py"
TUNNEL=Path("/root/botizin-tunnel/tunnel-client")
PROFILE=Path("/root/.config/tunnel-client/botizin-a22.yaml")
KEY=Path("/root/.botizin-control-plane-key")
MAPPER=ROOT/"botizin-mapa.py"
DIAGNOSTIC=ROOT/"diagnostico_supervisor.py"
LOG=ROOT/"supervisor.log"
STATE=ROOT/"supervisor-status.json"
PERIOD=12
UNREADY_GRACE=90   # ignore short startup, Wi-Fi blips and router reboots
RETRY_COOLDOWN=120 # no restart storms when the network is unavailable
MAP_INTERVAL=180
DIAG_INTERVAL=90  # background read-only snapshot; must not block recovery

def note(msg):
    with LOG.open("a",encoding="utf-8") as f:
        f.write(time.strftime("%Y-%m-%d %H:%M:%S ")+msg+"\n")

def listening(port):
    try:
        with socket.create_connection(("127.0.0.1",port),timeout=1):
            return True
    except OSError:
        return False

def tunnel_http():
    try:
        with urllib.request.urlopen("http://127.0.0.1:18080/readyz",timeout=3) as r:
            return r.status
    except urllib.error.HTTPError as exc:
        return exc.code
    except (OSError,urllib.error.URLError,ValueError):
        return 0

def running_commands():
    for proc in Path("/proc").iterdir():
        if not proc.name.isdigit():
            continue
        try:
            state=(proc/"stat").read_text().split(") ",1)[1][0]
            if state=="Z":continue
            args=[x.decode("utf-8","replace") for x in
                  (proc/"cmdline").read_bytes().split(bytes([0])) if x]
            if args:yield int(proc.name),args
        except (OSError,IndexError,ValueError):
            continue

def pids(kind):
    found=[]
    for pid,args in running_commands():
        if kind=="mcp":
            if args[0] in (str(PYTHON),str(PYTHON)+"3") and (str(SERVER) in args[1:] or "servidor.py" in args[1:]):
                found.append(pid)
        elif kind=="tunnel":
            if args[0]!=str(TUNNEL) or "run" not in args or "--profile" not in args:
                continue
            index=args.index("--profile")
            if index+1<len(args) and args[index+1]=="botizin-a22":
                found.append(pid)
    return found

def start_mcp():
    if not PYTHON.is_file() or not SERVER.is_file():
        note("MCP: missing existing Python or servidor.py; no installation attempted")
        return
    with (ROOT/"mcp-boot.log").open("ab") as log:
        proc=subprocess.Popen([str(PYTHON),str(SERVER)],cwd=ROOT,
            stdin=subprocess.DEVNULL,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
    note("MCP: starting PID "+str(proc.pid))

def start_tunnel():
    if not TUNNEL.is_file() or not PROFILE.is_file():
        note("TUNNEL: missing executable or profile; no installation attempted")
        return
    try:key=KEY.read_text(encoding="utf-8").strip()
    except OSError:
        note("TUNNEL: private key file absent; cannot auto-start")
        return
    if not key.startswith("sk-") or len(key)<30:
        note("TUNNEL: invalid private key format; refusing startup")
        return
    env=os.environ.copy()
    env["CONTROL_PLANE_API_KEY"]=key
    with (TUNNEL.parent/"tunnel-boot.log").open("ab") as log:
        proc=subprocess.Popen([str(TUNNEL),"run","--profile","botizin-a22",
              "--health.listen-addr","127.0.0.1:18080"],
              cwd=TUNNEL.parent,env=env,stdin=subprocess.DEVNULL,
              stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
    del key,env
    note("TUNNEL: starting PID "+str(proc.pid))

def terminate_verified(kind, pid):
    # Recheck identity immediately before sending a signal.
    if pid not in pids(kind):
        note(kind.upper()+": PID disappeared before safe stop")
        return False
    try:
        os.kill(pid,signal.SIGTERM)
    except OSError:
        note(kind.upper()+": failed to signal verified PID "+str(pid))
        return False
    for _ in range(12):
        time.sleep(0.5)
        if pid not in pids(kind):
            note(kind.upper()+": stopped verified PID "+str(pid))
            return True
    note(kind.upper()+": PID did not terminate gracefully; no SIGKILL attempted")
    return False

def write_state(mcp,tunnel,last_mcp,last_tunnel,network):
    record={"schema":2,"timestamp":time.strftime("%Y-%m-%dT%H:%M:%S%z"),
            "startup_order":["Android/VPN","Termux/SSH","Linux/MCP","tunnel","ESP32 discovery"],
            "mcp":{"port_8765":mcp,"pids":pids("mcp"),"last_recovery_age_s":int(time.monotonic()-last_mcp) if last_mcp else None},
            "tunnel":{"ready_http":tunnel,"pids":pids("tunnel"),"last_recovery_age_s":int(time.monotonic()-last_tunnel) if last_tunnel else None},
            "network_map":str(MAPPER),"network_mapping_running":network}
    temp=None
    try:
        with tempfile.NamedTemporaryFile("w",dir=STATE.parent,prefix=".supervisor-",delete=False) as out:
            temp=out.name
            os.chmod(temp,0o600)
            json.dump(record,out,indent=2,ensure_ascii=False)
            out.write("\n")
            out.flush()
            os.fsync(out.fileno())
        os.replace(temp,STATE)
    finally:
        if temp and os.path.exists(temp):os.unlink(temp)

def main():
    with Path("/root/.botizin-supervisor.lock").open("w") as lock:
        try:fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
        except BlockingIOError:return
        note("BOTIZIN v2 supervisor active. No firmware or credentials changed.")
        last_mcp=0
        last_tunnel=0
        mcp_missing_since=None
        tunnel_bad_since=None
        map_proc=None
        last_map=0
        diag_proc=None
        last_diag=0
        last_state=0
        previous=(None,None)
        while True:
            now=time.monotonic()
            mcp=listening(8765)
            mcp_pids=pids("mcp")
            health=tunnel_http()
            if (mcp,health)!=previous:
                note(f"STATUS: MCP={'UP' if mcp else 'DOWN'}; TUNNEL HTTP={health}")
                previous=(mcp,health)

            # 1. Do not start a second MCP if one already exists.
            if mcp:
                mcp_missing_since=None
            else:
                if mcp_missing_since is None:mcp_missing_since=now
                if now-last_mcp>RETRY_COOLDOWN:
                    if len(mcp_pids)==0:
                        start_mcp()
                        last_mcp=now
                    elif len(mcp_pids)==1 and now-mcp_missing_since>UNREADY_GRACE:
                        if terminate_verified("mcp",mcp_pids[0]):
                            start_mcp()
                        last_mcp=now
                    elif len(mcp_pids)>1:
                        note("MCP: multiple verified processes; no unsafe restart")
                        last_mcp=now

            # 2. No tunnel until MCP listens. 503 != healthy.
            if not mcp:
                tunnel_bad_since=None
            elif health==200:
                tunnel_bad_since=None
            else:
                if tunnel_bad_since is None:tunnel_bad_since=now
                # Fast cold start: if nothing is listening, no need to wait 90 seconds.
                if not pids("tunnel") and not listening(18080) and now-last_tunnel>20:
                    start_tunnel()
                    last_tunnel=now
                    tunnel_bad_since=now
                elif now-tunnel_bad_since>UNREADY_GRACE and now-last_tunnel>RETRY_COOLDOWN:
                    found=pids("tunnel")
                    if len(found)==1:
                        if terminate_verified("tunnel",found[0]):
                            time.sleep(2)
                            if not listening(18080):
                                start_tunnel()
                        last_tunnel=now
                    elif len(found)==0 and not listening(18080):
                        start_tunnel()
                        last_tunnel=now
                    else:
                        note("TUNNEL: ambiguous process/occupied port; no unsafe restart")
                        last_tunnel=now
                    tunnel_bad_since=now

            # 3. Discovery is read-only and never blocks startup of MCP/tunnel.
            if mcp and health==200 and MAPPER.is_file() and now-last_map>MAP_INTERVAL:
                if map_proc is None or map_proc.poll() is not None:
                    with (ROOT/"mapa.log").open("ab") as out:
                        map_proc=subprocess.Popen(["/usr/bin/python3",str(MAPPER)],
                            cwd=ROOT,stdin=subprocess.DEVNULL,
                            stdout=out,stderr=subprocess.STDOUT,start_new_session=True)
                    last_map=now

            # 4. Generate bounded diagnostic snapshot without blocking recovery.
            # No action on ESP32/MacroDroid/Termux; only local read-only queries.
            if DIAGNOSTIC.is_file() and now-last_diag>DIAG_INTERVAL:
                if diag_proc is None or diag_proc.poll() is not None:
                    with (ROOT/"diagnostico-boot.log").open("ab") as out:
                        diag_proc=subprocess.Popen(["/usr/bin/python3",str(DIAGNOSTIC)],
                            cwd=ROOT,stdin=subprocess.DEVNULL,
                            stdout=out,stderr=subprocess.STDOUT,start_new_session=True)
                    last_diag=now

            if now-last_state>30:
                try:write_state(mcp,health,last_mcp,last_tunnel,
                                map_proc is not None and map_proc.poll() is None)
                except OSError as e:note("STATE: unable to write status: "+type(e).__name__)
                last_state=now
            time.sleep(PERIOD)

if __name__=="__main__":main()
