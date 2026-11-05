#!/usr/bin/env python3
"""Install one LAN-only WROOM LED tool in the existing A22 MCP server.
Does not restart processes, change firmware, or access secrets.
"""
import ast
import datetime as dt
import os
from pathlib import Path
import re
import shutil
import sys
import tempfile

target = Path("/root/botizin-mcp/servidor.py")
if not target.is_file():
    sys.exit("STOP: servidor.py not found; nothing changed.")
original = target.read_text(encoding="utf-8")
if "def controlar_led_wroom(" in original:
    print("ALREADY_INSTALLED: controlar_led_wroom exists; nothing changed.")
    sys.exit(0)
try:
    tree = ast.parse(original)
except SyntaxError as exc:
    sys.exit(f"STOP: existing server has syntax error: {exc}")

tool_functions = [n for n in tree.body if isinstance(n, (ast.FunctionDef, ast.AsyncFunctionDef)) and n.name == "consultar_status_wroom"]
if len(tool_functions) != 1:
    sys.exit("STOP: expected exactly one consultar_status_wroom tool; nothing changed.")
tool = tool_functions[0]
tool_owner = None
for decorator in tool.decorator_list:
    method = decorator.func if isinstance(decorator, ast.Call) else decorator
    if isinstance(method, ast.Attribute) and method.attr == "tool" and isinstance(method.value, ast.Name):
        tool_owner = method.value.id
        break
if not tool_owner or not re.fullmatch(r"[A-Za-z_]\w*", tool_owner):
    sys.exit("STOP: MCP registration decorator not recognized; nothing changed.")

main_nodes = []
for n in tree.body:
    if isinstance(n, ast.If) and isinstance(n.test, ast.Compare):
        if isinstance(n.test.left, ast.Name) and n.test.left.id == "__name__":
            main_nodes.append(n)
if len(main_nodes) != 1:
    sys.exit("STOP: main guard not recognized; nothing changed.")
insert_line = main_nodes[0].lineno
lines = original.splitlines(keepends=True)
new_tool = '''
@TOOL_OWNER.tool()
def controlar_led_wroom(estado: str) -> dict:
    """Liga ou desliga o LED GPIO26 da WROOM via HTTP local. Ligar expira em 30s."""
    import ipaddress
    import re
    import urllib.request

    estado = estado.strip().lower()
    if estado not in ("ligar", "desligar"):
        return {"ok": False, "erro": "Use estado=ligar ou estado=desligar"}
    try:
        antigo = botizin_descobrir_status("wroom")
        if not isinstance(antigo, str) or "BOTIZIN WROOM V0.0.38" not in antigo:
            return {"ok": False, "erro": "WROOM 0.0.38 nao confirmada"}
        matched = re.search(r"(?m)^IP:\\s*(192\\.168\\.0\\.\\d{1,3})\\s*$", antigo)
        if not matched:
            return {"ok": False, "erro": "IP confiavel nao encontrado no status da WROOM"}
        ip = matched.group(1)
        if ipaddress.IPv4Address(ip) not in ipaddress.IPv4Network("192.168.0.0/24"):
            return {"ok": False, "erro": "IP fora da rede local permitida"}
        enabled = "1" if estado == "ligar" else "0"
        url = f"http://{ip}/led?enabled={enabled}"
        req = urllib.request.Request(url, data=b"", method="POST")
        with urllib.request.urlopen(req, timeout=10) as response:
            resposta = response.read(256).decode("utf-8", "replace").strip()
        esperado = "LED_STATE: ON" if estado == "ligar" else "LED_STATE: OFF"
        if esperado not in resposta:
            return {"ok": False, "erro": "WROOM nao confirmou o comando", "resposta": resposta}
        with urllib.request.urlopen(f"http://{ip}/status", timeout=8) as response:
            verificado = response.read(2048).decode("utf-8", "replace")
        confirmado = esperado in verificado
        return {
            "ok": confirmado, "origem": "BOTIZIN_WROOM_REAL",
            "ip": ip, "gpio": 26, "estado": "ON" if estado == "ligar" else "OFF",
            "auto_off_segundos": 30 if estado == "ligar" else 0,
            "resposta_placa": resposta,
            "confirmacao": "status verificado" if confirmado else "status nao confirmou",
        }
    except Exception as exc:
        return {"ok": False, "erro": type(exc).__name__ + ": " + str(exc)[:180]}


'''.replace("TOOL_OWNER", tool_owner)
candidate = "".join(lines[:insert_line-1]) + new_tool + "".join(lines[insert_line-1:])
try:
    compile(candidate, str(target), "exec")
except SyntaxError as exc:
    sys.exit(f"STOP: new code failed syntax check: {exc}")

stamp = dt.datetime.now(dt.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
backup = target.with_name(f"servidor.py.bak-led-{stamp}")
shutil.copy2(target, backup)
with tempfile.NamedTemporaryFile("w", encoding="utf-8", dir=target.parent, prefix=".servidor-led-", delete=False) as f:
    tmpname = f.name
    f.write(candidate)
try:
    os.chmod(tmpname, target.stat().st_mode & 0o777)
    os.replace(tmpname, target)
except BaseException:
    if os.path.exists(tmpname):
        os.unlink(tmpname)
    raise
print("INSTALADO: controlar_led_wroom(estado='ligar' ou 'desligar')")
print("BACKUP:", backup)
print("CODIGO VALIDADO: sim")
print("SERVIDOR REINICIADO: nao (precisa reiniciar para expor a ferramenta)")
