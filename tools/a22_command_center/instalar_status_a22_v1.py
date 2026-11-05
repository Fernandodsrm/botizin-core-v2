#!/usr/bin/env python3
"""BOTIZIN Command Center: instala a consulta A22 sem alterar as tres anteriores."""

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
CURRENT = {"consultar_status_botizin", "consultar_status_wroom", "consultar_status_ambas"}
NEW_TOOL = "consultar_status_a22"
PROCESS_PATTERN = r"^/root/botizin-mcp/\.venv/bin/python servidor\.py$"

TOOL_CODE = '''
@mcp.tool()
def consultar_status_a22() -> dict:
    """Consulta servicos e recursos do A22 sem ler ou expor credenciais."""
    import datetime
    import os
    import shutil
    import socket
    from pathlib import Path

    def porta_local(numero):
        try:
            with socket.create_connection(("127.0.0.1", numero), timeout=0.8):
                return True
        except OSError:
            return False

    def memoria_kb(arquivo, campo):
        try:
            for linha in Path(arquivo).read_text().splitlines():
                if linha.startswith(campo + ":"):
                    return int(linha.split()[1])
        except (OSError, ValueError, IndexError):
            pass
        return None

    def em_mb(kb):
        return round(kb / 1024, 1) if kb is not None else None

    try:
        disco = shutil.disk_usage("/root/botizin-mcp")
        disco_livre_gb = round(disco.free / (1024 ** 3), 2)
    except OSError:
        disco_livre_gb = None

    return {
        "origin": "BOTIZIN_A22",
        "ambiente": "Android Termux com Debian PRoot",
        "consultado_em_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(timespec="seconds"),
        "servicos_locais": {
            "mcp_porta_8765": porta_local(8765),
            "tunnel_health_porta_18080": porta_local(18080),
            "ssh_porta_8022": porta_local(8022),
        },
        "memoria_android_disponivel_mb": em_mb(memoria_kb("/proc/meminfo", "MemAvailable")),
        "memoria_processo_mcp_mb": em_mb(memoria_kb("/proc/self/status", "VmRSS")),
        "armazenamento_livre_gb": disco_livre_gb,
        "nota": "Portas TCP locais nao comprovam conectividade externa ou estado da VPN. Sem credenciais ou comandos remotos."
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
    a22 = functions[NEW_TOOL]()
    assert a22["origin"] == "BOTIZIN_A22"
    assert isinstance(a22["servicos_locais"]["mcp_porta_8765"], bool)
    assert "chave" not in str(a22).lower()
    s3 = functions["consultar_status_botizin"]()
    assert str(s3["status_texto"]).startswith("BOTIZIN CORE V")
    assert "wroom_status_texto" not in s3
    wroom = functions["consultar_status_wroom"]()
    assert str(wroom["status_texto"]).startswith("BOTIZIN WROOM V")
    both = functions["consultar_status_ambas"]()
    assert both["s3"]["ok"] and both["wroom"]["ok"]
    print("TESTES OK: S3 exclusiva, WROOM exclusiva, AMBAS e A22.", flush=True)
    print("A22:", a22["servicos_locais"], flush=True)

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

    fd, temp_file = tempfile.mkstemp(prefix=".a22-next-", suffix=".py", dir=ROOT)
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
        backup = ROOT / ("servidor.py.backup-a22-" + time.strftime("%Y%m%d-%H%M%S") + "-" + str(os.getpid()))
        shutil.copy2(ACTIVE, backup)
        os.chmod(backup, 0o600)
        print("BACKUP:", backup.name, flush=True)
        os.replace(temp, ACTIVE)
        changed = True
        print("INSTALADO. Reiniciando somente o MCP...", flush=True)
        restart_mcp()
        time.sleep(2)
        if wait_mcp():
            print("MCP ONLINE. CONCLUIDO: quarta ferramenta consultar_status_a22 registrada.", flush=True)
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
