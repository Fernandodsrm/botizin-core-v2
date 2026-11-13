#!/data/data/com.termux/files/usr/bin/bash
# Stage and run the S3->WROOM face relay without touching the A22 MCP,
# existing tmux bot, SSH, tunnel, MacroDroid or any OTA/partition.
set -euo pipefail
ACTION="${1:-}"
if [[ "$ACTION" != "preflight" && "$ACTION" != "start" ]]; then
  echo "STOP: expected preflight|start" >&2; exit 2
fi
command -v proot-distro >/dev/null || { echo "STOP: proot-distro missing"; exit 2; }
command -v curl >/dev/null || { echo "STOP: curl missing"; exit 2; }
command -v tmux >/dev/null || { echo "STOP: tmux missing"; exit 2; }
STAGE="$HOME/.local/share/botizin-voice-face"
mkdir -p "$STAGE"
SRC="https://raw.githubusercontent.com/Fernandodsrm/botizin-core-v2/1a4857a781b2e927f22c02bc84daf6ea545684aa/tools/a22/voice_face_relay.py"
TEMP="$STAGE/.voice_face_relay.py.new"
curl -fsSL --retry 2 --connect-timeout 8 --max-time 30 "$SRC" -o "$TEMP"
test -s "$TEMP"
proot-distro login botizin-linux -- python3 -m py_compile /root/botizin-mcp/botizin-supervisor.py
proot-distro login botizin-linux -- sh -c 'test -d /root/botizin-mcp || exit 3; umask 077; cat > /root/botizin-mcp/.voice_face_relay.py.new' < "$TEMP"
proot-distro login botizin-linux -- sh -c 'python3 -m py_compile /root/botizin-mcp/.voice_face_relay.py.new && mv /root/botizin-mcp/.voice_face_relay.py.new /root/botizin-mcp/voice_face_relay.py && chmod 600 /root/botizin-mcp/voice_face_relay.py'
mv "$TEMP" "$STAGE/voice_face_relay.py"
if [[ "$ACTION" == "preflight" ]]; then
  echo "A22: verifying S3 read-only state path..."
  proot-distro login botizin-linux -- python3 /root/botizin-mcp/voice_face_relay.py --once --dry-run
  echo "A22_PREFLIGHT_OK"
  exit 0
fi
if tmux has-session -t botizin-face 2>/dev/null; then
  echo "A22_FACE_SESSION_ALREADY_RUNNING"
else
  # A one-shot live POST proves WROOM .41 accepts A22-origin traffic before daemonizing.
  proot-distro login botizin-linux -- python3 /root/botizin-mcp/voice_face_relay.py --once
  tmux new-session -d -s botizin-face "proot-distro login botizin-linux -- python3 /root/botizin-mcp/voice_face_relay.py --watch >> '$STAGE/voice-face.log' 2>&1"
fi
sleep 3
if ! tmux has-session -t botizin-face 2>/dev/null; then
  echo "STOP: face tmux not running" >&2
  tail -n 10 "$STAGE/voice-face.log" 2>/dev/null || true
  exit 5
fi
echo "A22_FACE_RELAY_RUNNING: tmux botizin-face"
tail -n 4 "$STAGE/voice-face.log" 2>/dev/null || true
