#!/usr/bin/env python3
"""BOTIZIN Linux watchdog: preserves existing MCP and tunnel processes."""
import fcntl, os, socket, subprocess, time, signal, urllib.request, urllib.error
from pathlib import Path
ROOT=Path("/root/botizin-mcp")
BIN=ROOT/".venv/bin/python"
SERVER=ROOT/"servidor.py"
TUNNEL=Path("/root/botizin-tunnel/tunnel-client")
PROFILE=Path("/root/.config/tunnel-client/botizin-a22.yaml")
KEY=Path("/root/.botizin-control-plane-key")
LOG=ROOT/"supervisor.log"

def note(msg):
    with LOG.open("a") as f: f.write(time.strftime("%Y-%m-%d %H:%M:%S ")+msg+"\n")
def port(n):
    try:
        with socket.create_connection(("127.0.0.1", n),timeout=1):return True
    except OSError:return False
def tunnel_ready():
    """Only HTTP 200 on /readyz means ready. A listening port can be HTTP 503."""
    try:
        with urllib.request.urlopen("http://127.0.0.1:18080/readyz", timeout=3) as response:
            return response.status == 200
    except (OSError, urllib.error.URLError, urllib.error.HTTPError):
        return False

def known_tunnel_pids():
    """Find only the exact BOTIZIN tunnel executable and profile."""
    found=[]
    for proc in Path("/proc").iterdir():
        if not proc.name.isdigit():
            continue
        try:
            args=(proc/"cmdline").read_bytes().split(bytes([0]))
            args=[a.decode("utf-8","replace") for a in args if a]
        except OSError:
            continue
        if not args or args[0] != str(TUNNEL) or len(args) < 4:
            continue
        if "run" in args and "--profile" in args:
            i=args.index("--profile")
            if i+1 < len(args) and args[i+1] == "botizin-a22":
                found.append(int(proc.name))
    return found

def start_mcp():
    if not BIN.is_file() or not SERVER.is_file():
        note("MCP: Python ou servidor.py ausente");return None
    with (ROOT/"mcp-boot.log").open("ab") as f:
        p=subprocess.Popen([str(BIN),str(SERVER)],cwd=ROOT,stdin=subprocess.DEVNULL,
                           stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
    note("MCP: inicializacao, PID "+str(p.pid))
    return p
def start_tunnel():
    if not TUNNEL.is_file() or not PROFILE.is_file():
        note("TUNEL: executavel ou perfil ausente");return None
    try:key=KEY.read_text().strip()
    except OSError:
        note("TUNEL: chave privada nao cadastrada");return None
    if not key.startswith("sk-") or len(key)<30:
        note("TUNEL: formato de chave invalido");return None
    env=os.environ.copy()
    env["CONTROL_PLANE_API_KEY"]=key
    with Path("/root/botizin-tunnel/tunnel-boot.log").open("ab") as f:
        p=subprocess.Popen([str(TUNNEL),"run","--profile","botizin-a22",
                             "--health.listen-addr","127.0.0.1:18080"],
                           cwd=TUNNEL.parent,env=env,stdin=subprocess.DEVNULL,
                           stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
    del key,env
    note("TUNEL: inicializacao, PID "+str(p.pid))
    return p
def main():
    lock=Path("/root/.botizin-supervisor.lock").open("w")
    try:fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
    except BlockingIOError:return
    note("Supervisor Linux ativo")
    mcp=None;tunnel=None;last_mcp=0;last_tunnel=0;unready_since=None
    while True:
        now=time.monotonic()
        if not port(8765) and now-last_mcp>30:
            if mcp is None or mcp.poll() is not None:mcp=start_mcp()
            last_mcp=now
        if tunnel_ready():
            unready_since=None
        elif not port(8765):
            # Wait for the MCP server before starting the tunnel.
            unready_since=None
        else:
            if unready_since is None:
                unready_since=now
            if now-unready_since>=45 and now-last_tunnel>=45:
                if tunnel is not None and tunnel.poll() is None:
                    # Only terminate the tunnel process launched by this supervisor.
                    note("TUNEL: unhealthy for 45s; restarting supervised PID "+str(tunnel.pid))
                    tunnel.terminate()
                    try:
                        tunnel.wait(timeout=8)
                    except subprocess.TimeoutExpired:
                        note("TUNEL: graceful shutdown timeout; waiting before retry")
                        last_tunnel=now
                        time.sleep(12)
                        continue
                    tunnel=None
                    # Let the health port become free before the next attempt.
                    time.sleep(3)
                elif tunnel is None and port(18080):
                    matches=known_tunnel_pids()
                    if len(matches) == 1:
                        note("TUNEL: stale verified BOTIZIN process returning non-200; terminating PID "+str(matches[0]))
                        os.kill(matches[0], signal.SIGTERM)
                        time.sleep(4)
                        if port(18080):
                            note("TUNEL: health port still occupied; refusing duplicate startup")
                            last_tunnel=now
                            time.sleep(12)
                            continue
                    else:
                        note("TUNEL: health port occupied and known clients="+str(len(matches))+"; no unsafe kill")
                        last_tunnel=now
                        time.sleep(12)
                        continue
                elif tunnel is None and known_tunnel_pids():
                    # Existing client is still starting; avoid duplicate instances.
                    note("TUNEL: existing client detected; waiting for readiness")
                    last_tunnel=now
                    time.sleep(12)
                    continue
                tunnel=start_tunnel()
                last_tunnel=now
                unready_since=now
        time.sleep(12)
if __name__=="__main__":main()
