#!/data/data/com.termux/files/usr/bin/bash
# Stage A22 WROOM-040 module agent; run only once WROOM base v0.0.40 is physically verified.
# No firmware flashing, MCP/tunnel/SSH/Tailscale restart, or partition change.
set -euo pipefail

if [[ "$PREFIX" != "/data/data/com.termux/files/usr" ]]; then
  echo "STOP: execute no Termux nativo (~ \$)."
  exit 2
fi
command -v proot-distro >/dev/null || { echo "STOP: proot-distro ausente";exit 2; }
SRC="https://raw.githubusercontent.com/Fernandodsrm/botizin-core-v2/wroom/botizin-base-040/tools/a22"
STAGE="$HOME/.local/share/botizin"
mkdir -p "$STAGE"

echo "1/5 - Confirmando WROOM 0.0.40 em OTA0..."
proot-distro login botizin-linux -- python3 - <<'PY'
import ipaddress,json
from pathlib import Path
from urllib.request import build_opener,ProxyHandler
m=Path("/root/botizin-mcp/dispositivos.json")
try:
    w=json.loads(m.read_text())["dispositivos"]["wroom"]
    ip=w.get("ip") or w.get("last_known_ip")
    addr=ipaddress.ip_address(ip)
    assert addr.is_private and not addr.is_loopback and addr.version==4
    with build_opener(ProxyHandler({})).open(f"http://{addr}/status",timeout=5) as r:
        data=r.read(6000).decode("utf-8","replace")
    assert data.startswith("BOTIZIN WROOM V0.0.40\n"),"Version mismatch"
    assert "\nRUNNING_PARTITION: app0 / ota_0" in data,"Wrong base slot"
    assert "\nOTA: READY" in data,"OTA unavailable"
except Exception as e:
    raise SystemExit("STOP: WROOM 0.0.40 nao confirmada no OTA0: "+str(e)[:160])
print("WROOM 040 OTA0 OK")
PY

echo "2/5 - Baixando e validando agente..."
for item in wroom_module_agent.py enable_wroom_agent_supervisor.py; do
  curl -fsSL --retry 2 --max-time 30 "$SRC/$item" -o "$STAGE/$item"
  python -m py_compile "$STAGE/$item"
  proot-distro login botizin-linux -- sh -c "umask 077; cat > /root/botizin-mcp/.$item" < "$STAGE/$item"
done

echo "3/5 - Preparando supervisor com backup..."
proot-distro login botizin-linux -- bash -c '
set -euo pipefail
cd /root/botizin-mcp
python3 -m py_compile .wroom_module_agent.py .enable_wroom_agent_supervisor.py botizin-supervisor.py
mv .wroom_module_agent.py wroom_module_agent.py
chmod 700 wroom_module_agent.py
python3 .enable_wroom_agent_supervisor.py
python3 -m py_compile botizin-supervisor.py
'

echo "4/5 - Recarregando apenas supervisor Linux..."
proot-distro login botizin-linux -- python3 - <<'PY'
import os,signal,time
from pathlib import Path
script="/root/botizin-mcp/botizin-supervisor.py"
pids=[]
for entry in Path("/proc").iterdir():
    if not entry.name.isdigit():continue
    try:
        cmd=[b.decode("utf-8","replace") for b in (entry/"cmdline").read_bytes().split(bytes([0])) if b]
        if cmd and cmd[0] in ("python3","/usr/bin/python3") and script in cmd[1:]:
            pids.append(int(entry.name))
    except (OSError,ValueError):continue
if len(pids)!=1:
    raise SystemExit("STOP: supervisor PID nao e unico; nenhum reinicio feito: "+str(pids))
pid=pids[0]
os.kill(pid,signal.SIGTERM)
print("Supervisor sinalizado PID",pid)
for _ in range(20):
    time.sleep(.5)
    if not (Path("/proc")/str(pid)).exists():
        print("Supervisor antigo encerrou")
        break
else:
    raise SystemExit("STOP: supervisor ainda ativo; nova instancia nao iniciada")
PY
nohup proot-distro login botizin-linux -- python3 /root/botizin-mcp/botizin-supervisor.py \
  >> "$STAGE/watchdog-proot.log" 2>&1 </dev/null &

echo "5/5 - Verificando estado..."
sleep 6
proot-distro login botizin-linux -- python3 - <<'PY'
import json
from pathlib import Path
p=Path("/root/botizin-mcp/supervisor-status.json")
try:
    d=json.loads(p.read_text(encoding="utf-8"))
    assert d.get("schema")==2
except Exception as e:
    raise SystemExit("PENDENTE: estado supervisor nao confirmado: "+str(e))
print("SUPERVISOR",d.get("timestamp"))
print("MCP",d.get("mcp",{}).get("port_8765"))
print("TUNEL HTTP",d.get("tunnel",{}).get("ready_http"))
print("Agente instalado: a escolha fisica no menu autoriza uma instalacao especifica.")
PY
