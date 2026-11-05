#!/usr/bin/env python3
"""Safely restart only the BOTIZIN MCP server after adding the LED tool."""
from pathlib import Path
import os
import signal
import socket
import subprocess
import sys
import time
import shutil

folder=Path("/root/botizin-mcp")
source=folder/"servidor.py"
python=folder/".venv/bin/python"
log=folder/"mcp-boot.log"
backup=sorted(folder.glob("servidor.py.bak-led-*"), reverse=True)
if not source.is_file() or not python.is_file():
    sys.exit("STOP: server or its virtual Python not found. No changes.")
if "def controlar_led_wroom(" not in source.read_text():
    sys.exit("STOP: LED tool is not installed. No changes.")
try:
    compile(source.read_text(), str(source), "exec")
except SyntaxError as exc:
    sys.exit(f"STOP: code syntax error: {exc}")
if not backup:
    sys.exit("STOP: LED installer backup not found. No changes.")

def listen():
    try:
        with socket.create_connection(("127.0.0.1",8765),timeout=0.7):
            return True
    except OSError:
        return False

def server_pids():
    out=[]
    for p in Path("/proc").iterdir():
        if not p.name.isdigit() or int(p.name)==os.getpid(): continue
        try:
            args=(p/"cmdline").read_bytes().decode("utf-8","replace").split("\0")
            args=[a for a in args if a]
            if len(args)<2 or "python" not in Path(args[0]).name.lower(): continue
            if not any(Path(a).name=="servidor.py" for a in args[1:]): continue
            cwd=os.readlink(p/"cwd")
            if ("botizin-mcp" in cwd) or any("/botizin-mcp/servidor.py" in a for a in args[1:]):
                out.append(int(p.name))
        except (OSError,ValueError):
            continue
    return out

pids=server_pids()
print("MCP existing PIDs:",pids,flush=True)
if len(pids)>1:
    sys.exit("STOP: multiple MCP server processes; no restart to avoid disrupting the service.")
if not pids and listen():
    sys.exit("STOP: port 8765 occupied but server PID unknown; no restart.")
if len(pids)==1:
    old=pids[0]
    os.kill(old,signal.SIGTERM)
    for _ in range(25):
        if not server_pids() and not listen(): break
        time.sleep(0.2)
    else:
        sys.exit("STOP: existing MCP did not shut down cleanly; no new instance started.")

def start():
    with open(log,"ab") as fp:
        return subprocess.Popen([str(python),str(source)],cwd=folder,
            stdin=subprocess.DEVNULL,stdout=fp,stderr=subprocess.STDOUT,
            start_new_session=True)

def wait(process):
    for _ in range(40):
        if process.poll() is not None: return False
        if listen(): return True
        time.sleep(0.25)
    return False

proc=start()
if wait(proc):
    print("MCP REINICIADO COM SUCESSO - PID",proc.pid)
    print("TUNEL, SSH e WROOM NAO ALTERADOS")
    print("LED MCP: instalado no codigo; confirmar atualizacao do catalogo de ferramentas no ChatGPT")
    sys.exit(0)

print("Novo MCP falhou ao iniciar. Restaurando backup e tentando reativar o MCP anterior.",flush=True)
try:
    if proc.poll() is None:
        proc.terminate()
        proc.wait(timeout=5)
except Exception:
    sys.exit("ERROR: cannot stop failed MCP instance; manual inspection needed.")
shutil.copy2(backup[0],source)
oldproc=start()
if wait(oldproc):
    sys.exit("ROLLBACK: original MCP restored and running. LED tool disabled; report this output.")
sys.exit("ERROR: original MCP restore attempted but port 8765 did not respond. Inspect mcp-boot.log.")
