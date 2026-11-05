#!/usr/bin/env python3
"""BOTIZIN Command Center: adiciona diagnostico HTTP MacroDroid sem tocar nas quatro ferramentas."""

import ast
import os
import py_compile
import re
import shutil
import socket
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path("/root/botizin-mcp")
ACTIVE = ROOT / "servidor.py"
CURRENT = {"consultar_status_botizin", "consultar_status_wroom", "consultar_status_ambas", "consultar_status_a22"}
NEW_TOOL = "consultar_macrodroid"
PROCESS_PATTERN = r"^/root/botizin-mcp/\.venv/bin/python servidor\.py$"

TOOL_CODE = '''
@mcp.tool()
def consultar_macrodroid() -> dict:
    """Diagnostica o servidor HTTP local do MacroDroid sem acionar macros de controle."""
    import socket
    import urllib.error
    import urllib.request

    host = "127.0.0.1"
    porta = 8080
    caminho = "botizin_diagnostico_v1"
    assinatura = "BOTIZIN_MACRODROID_OK"
    dados = {
        "origin": "BOTIZIN_MACRODROID",
        "endereco_local": host,
        "porta": porta,
        "servidor_http_acessivel": False,
        "identidade_macrodroid_confirmada": False,
        "diagnostico": "porta_fechada",
        "nota": "Uma porta aberta nao identifica o MacroDroid. Teste somente de leitura."
    }
    try:
        with socket.create_connection((host, porta), timeout=1.0):
            dados["servidor_http_acessivel"] = True
    except OSError:
        return dados

    # Somente identificador de diagnostico, nunca URLs de macros que alteram o A22.
    url = "http://127.0.0.1:8080/" + caminho
    try:
        opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
        req = urllib.request.Request(url, method="GET",
            headers={"User-Agent": "Botizin-Healthcheck/1.0"})
        with opener.open(req, timeout=2.0) as resp:
            status = resp.status
            corpo = resp.read(128).decode("utf-8", "replace").strip()
        if status == 200 and corpo == assinatura:
            dados["identidade_macrodroid_confirmada"] = True
            dados["diagnostico"] = "resposta_de_diagnostico_confirmada"
        else:
            dados["diagnostico"] = "resposta_http_sem_assinatura"
    except urllib.error.HTTPError as exc:
        dados["diagnostico"] = "endpoint_de_diagnostico_ausente" if exc.code == 404 else "http_" + str(exc.code)
    except (urllib.error.URLError, OSError, ValueError):
        dados["diagnostico"] = "servidor_http_sem_resposta_valida"
    return dados


'''

def tools_registered(source):
    tree = ast.parse(source)
    names = set()
    for node in tree.body:
        if not isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
            continue
        for dec in node.decorator_list:
            if isinstance(dec, ast.Call) and isinstance(dec.func, ast.Attribute):
                attr = dec.func
                if isinstance(attr.value, ast.Name) and attr.value.id == "mcp" and attr.attr == "tool":
                    names.add(node.name)
    return names

def port_open(port):
    try:
        with socket.create_connection(("127.0.0.1", port), timeout=1):
            return True
    except OSError:
        return False

def wait_mcp(seconds=45):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        if port_open(8765):
            return True
        time.sleep(2)
    return False

def restart_mcp():
    subprocess.run(["pkill", "-f", PROCESS_PATTERN], check=False)

def smoke_test(path):
    import runpy
    functions = runpy.run_path(str(path))
    assert tools_registered(path.read_text(encoding="utf-8-sig")) == CURRENT | {NEW_TOOL}
    assert all(callable(functions[n]) for n in CURRENT | {NEW_TOOL})
    a22 = functions["consultar_status_a22"]()
    assert a22.get("origin") == "BOTIZIN_A22"
    macro = functions[NEW_TOOL]()
    assert macro.get("origin") == "BOTIZIN_MACRODROID"
    assert isinstance(macro.get("servidor_http_acessivel"), bool)
    assert isinstance(macro.get("identidade_macrodroid_confirmada"), bool)
    assert "chave" not in str(macro).lower()
    print("TESTES OK: 5 ferramentas, diagnostico local do A22 e HTTP somente leitura.", flush=True)
    print("MACRODROID:", macro.get("diagnostico"), flush=True)

def main():
    if not ACTIVE.is_file():
        print("PARADO: servidor.py nao encontrado. Nada alterado.")
        return 2
    try:
        source = ACTIVE.read_text(encoding="utf-8-sig")
        existing = tools_registered(source)
    except (OSError, SyntaxError) as error:
        print("PARADO: arquivo atual invalido:", type(error).__name__)
        return 2
    if existing == CURRENT | {NEW_TOOL}:
        print("JA INSTALADO: quarta ferramenta registrada. Nenhuma alteracao.")
        return 0
    if existing != CURRENT:
        print("PARADO: ferramentas diferentes das esperadas:", sorted(existing))
        return 2
    if "wroom_status_texto" in source:
        print("PARADO: agregacao antiga ainda presente; nada alterado.")
        return 2
    match = list(re.finditer(r'(?m)^if __name__\s*==\s*[\'"]__main__[\'"]\s*:', source))
    if len(match) != 1:
        print("PARADO: nao localizei um unico ponto de inicializacao.")
        return 2
    candidate = source[:match[0].start()].rstrip() + "\n\n\n" + TOOL_CODE + source[match[0].start():]
    if tools_registered(candidate) != CURRENT | {NEW_TOOL}:
        print("PARADO: conjunto de ferramentas invalido.")
        return 2

    fd, temp_file = tempfile.mkstemp(prefix=".macro-http-next-", suffix=".py", dir=ROOT)
    temp = Path(temp_file)
    backup = None
    changed = False
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as handle:
            handle.write(candidate)
            handle.flush()
            os.fsync(handle.fileno())
        os.chmod(temp, 0o600)
        py_compile.compile(str(temp), doraise=True)
        if not port_open(8765):
            print("PARADO: MCP nao escuta na 8765. Nada instalado.")
            return 2
        print("VALIDACAO OK. Executando testes locais de leitura...", flush=True)
        smoke_test(temp)
        backup = ROOT / ("servidor.py.backup-macrodroid-" + time.strftime("%Y%m%d-%H%M%S") + "-" + str(os.getpid()))
        shutil.copy2(ACTIVE, backup)
        os.chmod(backup, 0o600)
        print("BACKUP:", backup.name, flush=True)
        os.replace(temp, ACTIVE)
        changed = True
        print("INSTALADO. Reiniciando somente o MCP...", flush=True)
        restart_mcp()
        time.sleep(2)
        if wait_mcp():
            print("MCP ONLINE. CONCLUIDO: quinta ferramenta consultar_macrodroid registrada.", flush=True)
            print("PROXIMO: ChatGPT > Plugins > BOTIZIN S3 PRIVADO > Atualizar ferramentas.", flush=True)
            return 0
        print("FALHA: MCP nao voltou. Restaurando backup...", flush=True)
        shutil.copy2(backup, ACTIVE)
        os.chmod(ACTIVE, 0o600)
        restart_mcp()
        print("BACKUP RESTAURADO.", "MCP online." if wait_mcp() else "Verificar mcp-boot.log.", flush=True)
        return 1
    except Exception as error:
        print("ERRO:", type(error).__name__, str(error)[:180], flush=True)
        if changed and backup is not None and backup.exists():
            shutil.copy2(backup, ACTIVE)
            os.chmod(ACTIVE, 0o600)
            restart_mcp()
            print("BACKUP RESTAURADO; validar servico.", flush=True)
        return 1
    finally:
        temp.unlink(missing_ok=True)

if __name__ == "__main__":
    sys.exit(main())
