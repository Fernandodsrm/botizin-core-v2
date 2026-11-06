#!/data/data/com.termux/files/usr/bin/bash
# Install BOTIZIN auto-recovery on native Termux (not inside botizin-linux).
set -euo pipefail
if [[ "$PREFIX" != "/data/data/com.termux/files/usr" ]]; then
  echo "ERRO: execute no Termux nativo (~ \$), nao dentro de root@localhost."; exit 1
fi
command -v proot-distro >/dev/null || { echo "ERRO: proot-distro nao encontrado"; exit 1; }
command -v python >/dev/null || { echo "ERRO: Python Termux nao encontrado"; exit 1; }
BASE="$HOME/.local/share/botizin"
URL="https://raw.githubusercontent.com/Fernandodsrm/botizin-core-v2/main/tools/a22"
mkdir -p "$BASE" "$HOME/.termux/boot"
echo "1. Baixando supervisores (nao altera firmware ou servidor.py)..."
curl -fsSL --max-time 30 "$URL/supervisor_termux.py" -o "$BASE/watchdog_host.py"
curl -fsSL --max-time 30 "$URL/supervisor_linux.py" -o "$BASE/supervisor_linux.py"
python -m py_compile "$BASE/watchdog_host.py"
chmod 700 "$BASE/watchdog_host.py"
echo "2. Preparando Linux, sem encerrar os processos existentes..."
proot-distro login botizin-linux -- python3 -m py_compile /root/botizin-mcp/servidor.py
proot-distro login botizin-linux -- bash -c 'test -d /root/botizin-mcp || exit 2; umask 077; cat > /root/botizin-mcp/botizin-supervisor.py; python3 -m py_compile /root/botizin-mcp/botizin-supervisor.py; chmod 700 /root/botizin-mcp/botizin-supervisor.py' < "$BASE/supervisor_linux.py"
BOOT="$HOME/.termux/boot/10-botizin.sh"
if [ -e "$BOOT" ]; then cp -p "$BOOT" "$BOOT.bak-$(date +%Y%m%d%H%M%S)"; fi
cat > "$BOOT" <<'BOOTCODE'
#!/data/data/com.termux/files/usr/bin/sh
termux-wake-lock
nohup /data/data/com.termux/files/usr/bin/python "$HOME/.local/share/botizin/watchdog_host.py" >> "$HOME/.local/share/botizin/termux-boot.log" 2>&1 < /dev/null &
BOOTCODE
chmod 700 "$BOOT"
echo "3. Credencial: necessaria para o tunel reiniciar sozinho."
echo "A chave sera salva SOMENTE no A22, com permissao 600, nunca no GitHub."
read -r -s -p "Cole sua chave da API (Enter para pular): " CHAVE
echo
if [ -n "$CHAVE" ]; then
  if [[ "$CHAVE" == sk-* ]] && (( $(printf '%s' "$CHAVE" | wc -c) > 30 )); then
    printf '%s\n' "$CHAVE" | proot-distro login botizin-linux -- sh -c 'umask 077; cat > /root/.botizin-control-plane-key; chmod 600 /root/.botizin-control-plane-key'
    echo "Credencial privada cadastrada."
  else
    echo "Chave com formato inesperado; nada salvo."
  fi
else
  echo "Chave nao alterada; sem chave persistida, tunel pode nao reiniciar."
fi
unset CHAVE
echo "4. Iniciando agora..."
termux-wake-lock >/dev/null 2>&1 || true
sshd || true
nohup python "$BASE/watchdog_host.py" >> "$BASE/watchdog-host.log" 2>&1 < /dev/null &
sleep 3
echo "Preparacao concluida. Para funcionar apos reinicio, instale e abra Termux:Boot uma vez."
echo "No Android: Tailscale = VPN sempre ativa; Termux e Termux:Boot = bateria sem restricoes."
echo "Log de recuperacao: tail -n 12 ~/.local/share/botizin/watchdog-host.log"
