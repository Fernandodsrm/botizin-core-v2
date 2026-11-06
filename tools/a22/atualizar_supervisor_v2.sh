#!/data/data/com.termux/files/usr/bin/bash
# BOTIZIN v2: preserve MCP, SSH, Tailscale, OTA and credentials.
set -euo pipefail
if [[ "$PREFIX" != "/data/data/com.termux/files/usr" ]]; then
  echo "ERRO: use o Termux nativo (~ $), nao root@localhost"; exit 1
fi
command -v proot-distro >/dev/null || { echo "ERRO: proot-distro ausente"; exit 1; }
command -v python >/dev/null || { echo "ERRO: Python Termux ausente"; exit 1; }
BASE="$HOME/.local/share/botizin"
URL="https://raw.githubusercontent.com/Fernandodsrm/botizin-core-v2/main/tools/a22"
mkdir -p "$BASE"
echo "1/5 - Baixando supervisor validado e mapa da rede..."
curl -fsSL --retry 2 --max-time 30 "$URL/supervisor_linux.py" -o "$BASE/.supervisor-v2.new.py"
curl -fsSL --retry 2 --max-time 30 "$URL/mapear_rede.py" -o "$BASE/.map-v2.new.py"
python -m py_compile "$BASE/.supervisor-v2.new.py" "$BASE/.map-v2.new.py"

echo "2/5 - Validando no Linux, sem afetar processos..."
proot-distro login botizin-linux -- sh -c 'umask 077; cat > /root/botizin-mcp/.supervisor-v2.new' < "$BASE/.supervisor-v2.new.py"
proot-distro login botizin-linux -- sh -c 'umask 077; cat > /root/botizin-mcp/.map-v2.new' < "$BASE/.map-v2.new.py"
proot-distro login botizin-linux -- python3 -m py_compile /root/botizin-mcp/.supervisor-v2.new /root/botizin-mcp/.map-v2.new

echo "3/5 - Backup e troca atomica apenas dos scripts de supervisao..."
proot-distro login botizin-linux -- bash -c '
  set -e
  cd /root/botizin-mcp
  T=$(date -u +%Y%m%dT%H%M%SZ)
  test -f botizin-supervisor.py
  cp -p botizin-supervisor.py "botizin-supervisor.py.bak-$T"
  if test -f botizin-mapa.py; then cp -p botizin-mapa.py "botizin-mapa.py.bak-$T"; fi
  mv .supervisor-v2.new botizin-supervisor.py
  mv .map-v2.new botizin-mapa.py
  chmod 700 botizin-supervisor.py botizin-mapa.py
  echo "Backup do supervisor criado, versao v2 instalada."
'

echo "4/5 - Pedindo recarga SOMENTE do supervisor Linux..."
proot-distro login botizin-linux -- python3 - <<'PY'
import os,signal
from pathlib import Path
targets=[]
for p in Path("/proc").iterdir():
    if not p.name.isdigit() or int(p.name)==os.getpid():
        continue
    try:
        args=[a.decode("utf-8","replace") for a in
              (p/"cmdline").read_bytes().split(bytes([0])) if a]
        if len(args)>1 and args[0].endswith("python3") and (
            "/root/botizin-mcp/botizin-supervisor.py" in args[1:]):
            targets.append(int(p.name))
    except (OSError,ValueError):
        pass
if len(targets)==1:
    os.kill(targets[0],signal.SIGTERM)
    print("Supervisor antigo sinalizado PID",targets[0])
elif len(targets)==0:
    print("Nenhum supervisor antigo encontrado; inicializando novo.")
else:
    print("Mais de um supervisor antigo; nenhum processo sera encerrado:",len(targets))
PY
sleep 3
# A daemon owned by the Termux watchdog may already have relaunched.
# The Linux flock prevents two supervisors from managing the same services.
nohup proot-distro login botizin-linux -- python3 /root/botizin-mcp/botizin-supervisor.py \
  >> "$BASE/watchdog-proot.log" 2>&1 < /dev/null &
echo "Supervisor v2 solicitado; nenhuma placa ou firmware foi alterado."

echo "5/5 - Aguarde e confira o resumo..."
sleep 12
proot-distro login botizin-linux -- bash -c '
  echo === MCP ===
  curl -sS --max-time 3 -o /dev/null -w "PORTA 8765: HTTP %{http_code}\n" http://127.0.0.1:8765/mcp || true
  echo === TUNEL ===
  curl -sS --max-time 3 -o /dev/null -w "TUNEL /readyz: HTTP %{http_code}\n" http://127.0.0.1:18080/readyz || true
  echo === SUPERVISOR ===
  tail -n 8 /root/botizin-mcp/supervisor.log || true
  echo === MAPA ===
  if test -s /root/botizin-mcp/dispositivos.json; then
    python3 -c "import json; j=json.load(open(\"/root/botizin-mcp/dispositivos.json\")); print([(k,v.get(\"ip\") if v.get(\"online\") else \"offline\") for k,v in j[\"dispositivos\"].items()])"
  else
    echo "Mapeamento aguardando primeira coleta."
  fi
'
echo "Atualizacao aplicada. O acesso SSH do Termux nao foi reiniciado."
