#!/usr/bin/env python3
"""BOTIZIN Linux watchdog: preserves existing MCP and tunnel processes."""
import fcntl, os, socket, subprocess, time
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
    mcp=None;tunnel=None;last_mcp=0;last_tunnel=0
    while True:
        now=time.monotonic()
        if not port(8765) and now-last_mcp>30:
            if mcp is None or mcp.poll() is not None:mcp=start_mcp()
            last_mcp=now
        if not port(18080) and now-last_tunnel>30:
            if tunnel is None or tunnel.poll() is not None:tunnel=start_tunnel()
            last_tunnel=now
        time.sleep(12)
if __name__=="__main__":main()
