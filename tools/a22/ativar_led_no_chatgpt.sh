#!/data/data/com.termux/files/usr/bin/bash
# BOTIZIN A22: instala/ativa ferramenta de LED no MCP ja existente.
# Nao reinicia Android/SSH/Tailscale/ESP32 e nao altera credenciais.
set -euo pipefail
if [[ "$PREFIX" != "/data/data/com.termux/files/usr" ]]; then
  echo "ERRO: execute no Termux nativo (~ \$), nao no Linux root@localhost."
  exit 1
fi

BASE="$HOME/.local/share/botizin"
mkdir -p "$BASE"
URL="https://raw.githubusercontent.com/Fernandodsrm/botizin-core-v2/main/tools/a22"
echo "1/4: Verificando instalador da ferramenta LED..."
curl -fsSL --retry 2 --max-time 30 "$URL/install_wroom_led_mcp.py" -o "$BASE/install_wroom_led_mcp.py"
proot-distro login botizin-linux -- sh -c 'umask 077; cat > /root/botizin-mcp/.install_wroom_led_mcp.py' < "$BASE/install_wroom_led_mcp.py"
proot-distro login botizin-linux -- python3 /root/botizin-mcp/.install_wroom_led_mcp.py

echo "2/4: Confirmando registro no MCP e supervisor v2..."
proot-distro login botizin-linux -- python3 - <<'PY'
from pathlib import Path
import ast
p=Path("/root/botizin-mcp/servidor.py")
tree=ast.parse(p.read_text(encoding="utf-8"))
matches=[n for n in tree.body if isinstance(n,(ast.FunctionDef,ast.AsyncFunctionDef)) and n.name=="controlar_led_wroom"]
if len(matches)!=1 or not matches[0].decorator_list:
    raise SystemExit("STOP: ferramenta LED nao registrada corretamente; servidor preservado.")
sup=Path("/root/botizin-mcp/botizin-supervisor.py")
if not sup.is_file() or "BOTIZIN Linux supervisor v2" not in sup.read_text(encoding="utf-8"):
    raise SystemExit("STOP: supervisor v2 nao encontrado; servidor preservado.")
print("Ferramenta LED e supervisor v2 confirmados.")
PY

echo "3/4: Recarregando somente o processo MCP..."
proot-distro login botizin-linux -- python3 - <<'PY'
from pathlib import Path
import os,signal
python="/root/botizin-mcp/.venv/bin/python"
target="/root/botizin-mcp/servidor.py"
pids=[]
for p in Path("/proc").iterdir():
    if not p.name.isdigit():continue
    try:
        cmd=[a.decode("utf-8","replace") for a in (p/"cmdline").read_bytes().split(bytes([0])) if a]
        if cmd and cmd[0] in (python,python+"3") and target in cmd[1:]:
            pids.append(int(p.name))
    except (OSError,ValueError):continue
if len(pids)>1:
    raise SystemExit("STOP: ha varios MCPs, nenhum reiniciado: "+str(pids))
if pids:
    os.kill(pids[0],signal.SIGTERM)
    print("PID MCP antigo sinalizado:",pids[0])
else:
    print("MCP sem PID: supervisor fara inicializacao.")
PY

echo "4/4: Aguardando o supervisor recuperar MCP e tunel..."
for i in $(seq 1 30); do
  sleep 5
  RESULT="$(proot-distro login botizin-linux -- python3 - <<'PY'
import socket,urllib.request,urllib.error
try:
    with socket.create_connection(("127.0.0.1",8765),timeout=2):mcp=1
except OSError:mcp=0
try:
    with urllib.request.urlopen("http://127.0.0.1:18080/readyz",timeout=3) as r:ready=r.status
except urllib.error.HTTPError as e:ready=e.code
except (OSError,urllib.error.URLError):ready=0
print(str(mcp)+" "+str(ready))
PY
  )"
  echo "MCP/tunel: $RESULT"
  if [[ "$RESULT" == "1 200" ]]; then
    echo "OK: MCP e tunel responderam. Sincronize ferramentas do plugin BOTIZIN S3 PRIVADO no ChatGPT."
    echo "Nova ferramenta: controlar_led_wroom(estado='ligar' ou 'desligar')."
    exit 0
  fi
done
echo "PENDENTE: conexao nao recuperou em 150 s. Verifique supervisor.log e mcp-boot.log no A22."
exit 2
