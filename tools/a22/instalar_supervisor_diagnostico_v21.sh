#!/data/data/com.termux/files/usr/bin/bash
# BOTIZIN supervisor v2.1: diagnostics add-on. Run from native Termux (~ $).
# No firmware, SSH, Tailscale, credentials, network configuration modifications.
set -euo pipefail

if [[ "$PREFIX" != "/data/data/com.termux/files/usr" ]]; then
  echo "STOP: use o Termux nativo (~ \$), nao Linux root@localhost."
  exit 2
fi
command -v proot-distro >/dev/null || { echo "STOP: proot-distro ausente"; exit 2; }
command -v python >/dev/null || { echo "STOP: Python do Termux ausente"; exit 2; }
BASE="$HOME/.local/share/botizin"
URL="https://raw.githubusercontent.com/Fernandodsrm/botizin-core-v2/main/tools/a22"
mkdir -p "$BASE"

echo "1/5 - Baixando e validando scripts (sem alterar servicos)..."
for item in supervisor_linux.py diagnostico_supervisor.py instalar_diagnostico_mcp.py; do
  curl -fsSL --retry 2 --max-time 35 "$URL/$item" -o "$BASE/.diag21-$item"
  python -m py_compile "$BASE/.diag21-$item"
done

echo "2/5 - Validando no Debian do A22..."
for item in supervisor_linux.py diagnostico_supervisor.py instalar_diagnostico_mcp.py; do
  proot-distro login botizin-linux -- sh -c "umask 077; cat > /root/botizin-mcp/.diag21-$item" < "$BASE/.diag21-$item"
done
proot-distro login botizin-linux -- python3 -m py_compile \
  /root/botizin-mcp/.diag21-supervisor_linux.py \
  /root/botizin-mcp/.diag21-diagnostico_supervisor.py \
  /root/botizin-mcp/.diag21-instalar_diagnostico_mcp.py

echo "3/5 - Backup e instalacao conservadora..."
proot-distro login botizin-linux -- bash -c '
set -euo pipefail
cd /root/botizin-mcp
T=$(date -u +%Y%m%dT%H%M%SZ)
test -f botizin-supervisor.py
test -f servidor.py
python3 -m py_compile servidor.py
cp -p botizin-supervisor.py "botizin-supervisor.py.bak-diag21-$T"
if test -f diagnostico_supervisor.py; then cp -p diagnostico_supervisor.py "diagnostico_supervisor.py.bak-$T"; fi
python3 .diag21-instalar_diagnostico_mcp.py
mv .diag21-supervisor_linux.py botizin-supervisor.py
mv .diag21-diagnostico_supervisor.py diagnostico_supervisor.py
chmod 700 botizin-supervisor.py diagnostico_supervisor.py
python3 -m py_compile botizin-supervisor.py diagnostico_supervisor.py servidor.py
echo "Arquivos instalados e compilados. Nenhum firmware foi alterado."
'

echo "4/5 - Recarregando SOMENTE MCP e supervisor Linux..."
proot-distro login botizin-linux -- python3 - <<'PY'
import os,signal
from pathlib import Path
srv="/root/botizin-mcp/servidor.py"
mcp="/root/botizin-mcp/.venv/bin/python"
supervisor="/root/botizin-mcp/botizin-supervisor.py"
found={"mcp":[],"supervisor":[]}
for p in Path("/proc").iterdir():
    if not p.name.isdigit() or int(p.name)==os.getpid():continue
    try:
        cmd=[a.decode("utf-8","replace") for a in (p/"cmdline").read_bytes().split(bytes([0])) if a]
        if not cmd:continue
        if cmd[0] in (mcp,mcp+"3") and srv in cmd[1:]:found["mcp"].append(int(p.name))
        if (cmd[0].endswith("/python3") or cmd[0]=="python3") and supervisor in cmd[1:]:
            found["supervisor"].append(int(p.name))
    except (OSError,ValueError):pass
if len(found["mcp"])>1 or len(found["supervisor"])>1:
    raise SystemExit("STOP: processos duplicados. Nada sera sinalizado: "+str(found))
for kind in ("supervisor","mcp"):
    for pid in found[kind]:
        os.kill(pid,signal.SIGTERM)
        print(kind, "PID",pid,"sinalizado para recarga")
PY
sleep 3
nohup proot-distro login botizin-linux -- python3 /root/botizin-mcp/botizin-supervisor.py \
  >> "$BASE/watchdog-proot.log" 2>&1 < /dev/null &

echo "5/5 - Aguardando diagnostico e retorno da conexao..."
for i in $(seq 1 24); do
  sleep 5
  STATE="$(proot-distro login botizin-linux -- python3 - <<'PY'
import json,socket
from pathlib import Path
try:
    with socket.create_connection(("127.0.0.1",8765),timeout=1):mcp=1
except OSError:mcp=0
path=Path("/root/botizin-mcp/diagnostico-supervisor.json")
try:
    info=json.loads(path.read_text(encoding="utf-8"))
    ok=int(info.get("origem")=="BOTIZIN_SUPERVISOR_DIAGNOSTICO")
    http=info.get("servicos",{}).get("tunel_readyz_http")
except (OSError,ValueError,TypeError):
    ok=0
    http="?"
print("MCP="+str(mcp)+" DIAGNOSTICO="+str(ok)+" TUNEL_HTTP="+str(http))
PY
  )"
  echo "$STATE"
  if [[ "$STATE" == *"MCP=1 DIAGNOSTICO=1 TUNEL_HTTP=200"* ]]; then
    echo "CONCLUIDO: MCP/tunel online e diagnostico gerado."
    echo "A consulta consultar_status_a22 passa a incluir diagnostico_supervisor."
    echo "Se nao aparecer no ChatGPT, atualize ferramentas do plugin BOTIZIN S3 PRIVADO."
    exit 0
  fi
done
echo "ATENCAO: instalacao concluida, mas ainda sem confirmacao completa de MCP + diagnostico + tunel."
echo "Conferir supervisor.log e diagnostico-boot.log no A22. Nao reexecute sem diagnosticar."
exit 1
