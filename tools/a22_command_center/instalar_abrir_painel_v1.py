#!/usr/bin/env python3
"""BOTIZIN Command Center: adiciona comando controlado HTTP sem criar macros Android."""

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
CURRENT = {"consultar_status_botizin", "consultar_status_wroom", "consultar_status_ambas", "consultar_status_a22", "consultar_macrodroid"}
NEW_TOOL = "abrir_painel_botizin"
PROCESS_PATTERN = r"^/root/botizin-mcp/\.venv/bin/python servidor\.py$"

TOOL_CODE = '''
@mcp.tool()
def abrir_painel_botizin() -> dict:
    """ACAO no A22: solicita abertura pelo gatilho HTTP existente. Usar so quando o usuario pedir expressamente."""
    import urllib.error
    import urllib.request

    # Recusa acionar rotas HTTP caso nao seja o servidor do MacroDroid verificado.
    diagnostico = consultar_macrodroid()
    if not diagnostico.get("identidade_macrodroid_confirmada", False):
        return {
            "origin": "BOTIZIN_COMANDO",
            "solicitacao_enviada": False,
            "motivo": "macro_diagnostico_nao_confirmada",
            "observacao": "Nenhuma acao enviada ao A22."
        }

    # URL fixa, acesso SOMENTE via loopback do A22. Sem URL/comando arbitrario.
    url = "http://127.0.0.1:8080/botizin-abrir"

    class BloquearRedirecionamento(urllib.request.HTTPRedirectHandler):
        def redirect_request(self, req, fp, code, msg, headers, newurl):
            return None

    abridor = urllib.request.build_opener(
        urllib.request.ProxyHandler({}),
        BloquearRedirecionamento()
    )
    req = urllib.request.Request(url, method="GET",
        headers={"User-Agent": "Botizin-Acao-Autorizada/1.0"})
    try:
        with abridor.open(req, timeout=4.0) as resposta:
            codigo = resposta.status
        return {
            "origin": "BOTIZIN_COMANDO",
            "solicitacao_enviada": (200 <= codigo < 300),
            "codigo_http": codigo,
            "comando": "abrir_painel_botizin",
            "observacao": "HTTP recebido nao comprova que o Android abriu a tela. A macro existente exige tela ligada e aparelho desbloqueado."
        }
    except urllib.error.HTTPError as erro:
        return {
            "origin": "BOTIZIN_COMANDO",
            "solicitacao_enviada": False,
            "codigo_http": erro.code,
            "motivo": "servidor_recusou_ou_redirecionou"
        }
    except (urllib.error.URLError, OSError, ValueError) as erro:
        return {
            "origin": "BOTIZIN_COMANDO",
            "solicitacao_enviada": False,
            "motivo": type(erro).__name__,
            "observacao": "Falha de comunicacao com gatilho existente. Sem comandos adicionais."
        }


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
    macro = functions["consultar_macrodroid"]()
    assert macro.get("origin") == "BOTIZIN_MACRODROID"
    assert isinstance(macro.get("identidade_macrodroid_confirmada"), bool)
    # NUNCA executar abrir_painel_botizin durante instalacao/testes.
    print("TESTES OK: 6 ferramentas preservadas e comando disponivel.", flush=True)
    print("MACRODROID:", macro.get("diagnostico"), flush=True)
    print("COMANDO NAO EXECUTADO. Aguardar pedido do usuario pelo ChatGPT.", flush=True)

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
        print("JA INSTALADO: sexto comando registrado. Nenhuma alteracao.")
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

    fd, temp_file = tempfile.mkstemp(prefix=".macro-open-next-", suffix=".py", dir=ROOT)
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
        backup = ROOT / ("servidor.py.backup-abrir-painel-" + time.strftime("%Y%m%d-%H%M%S") + "-" + str(os.getpid()))
        shutil.copy2(ACTIVE, backup)
        os.chmod(backup, 0o600)
        print("BACKUP:", backup.name, flush=True)
        os.replace(temp, ACTIVE)
        changed = True
        print("INSTALADO. Reiniciando somente o MCP...", flush=True)
        restart_mcp()
        time.sleep(2)
        if wait_mcp():
            print("MCP ONLINE. CONCLUIDO: sexta ferramenta abrir_painel_botizin registrada.", flush=True)
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
