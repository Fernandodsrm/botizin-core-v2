#!/usr/bin/env python3
"""BOTIZIN: corrige IPs S3/WROOM, com testes e rollback, sem alterar firmwares."""

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
from urllib.request import ProxyHandler, build_opener

ROOT = Path("/root/botizin-mcp")
ACTIVE = ROOT / "servidor.py"
EXPECTED = {
    "consultar_status_botizin",
    "consultar_status_wroom",
    "consultar_status_ambas",
    "consultar_status_a22",
}
OLD_NEW = {
    "http://192.168.0.7/status": "http://192.168.0.2/status",
    "http://192.168.0.16/status": "http://192.168.0.10/status",
}
PROCESS_PATTERN = r"^/root/botizin-mcp/\.venv/bin/python servidor\.py$"


def registered_tools(source):
    tree = ast.parse(source)
    result = set()
    for node in tree.body:
        if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
            for d in node.decorator_list:
                if (isinstance(d, ast.Call) and isinstance(d.func, ast.Attribute)
                        and isinstance(d.func.value, ast.Name)
                        and d.func.value.id == "mcp" and d.func.attr == "tool"):
                    result.add(node.name)
    return result


def verify_board(url, prefix):
    opener = build_opener(ProxyHandler({}))
    with opener.open(url, timeout=5) as response:
        text = response.read(1000).decode("utf-8", "replace")
    if not text.startswith(prefix):
        raise RuntimeError("Dispositivo nao corresponde ao IP: " + url)
    return text.splitlines()[0]


def local_port_open(port):
    try:
        with socket.create_connection(("127.0.0.1", port), timeout=1):
            return True
    except OSError:
        return False


def wait_mcp(seconds=45):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        if local_port_open(8765):
            return True
        time.sleep(2)
    return False


def restart_mcp():
    # Somente o processo Python; supervisor Termux o relanca.
    subprocess.run(["pkill", "-f", PROCESS_PATTERN], check=False)


def smoke_test(file):
    import runpy
    funcs = runpy.run_path(str(file))
    a = funcs["consultar_status_botizin"]()
    b = funcs["consultar_status_wroom"]()
    assert str(a.get("status_texto", "")).startswith("BOTIZIN CORE V"), "S3 invalida"
    assert "wroom_status_texto" not in a, "S3 deixou de ser independente"
    assert str(b.get("status_texto", "")).startswith("BOTIZIN WROOM V"), "WROOM invalida"
    combined = funcs["consultar_status_ambas"]()
    assert combined["s3"]["ok"] and combined["wroom"]["ok"], "Consulta conjunta invalida"
    diag = funcs["consultar_status_a22"]()
    assert diag.get("origin") == "BOTIZIN_A22", "Diagnostico A22 invalido"
    print("TESTES OK: S3, WROOM, AMBAS e A22.", flush=True)
    print("S3:", a["status_texto"].splitlines()[0], flush=True)
    print("WROOM:", b["status_texto"].splitlines()[0], flush=True)


def main():
    if not ACTIVE.is_file():
        print("PARADO: servidor.py nao encontrado; nada alterado.")
        return 2
    try:
        source = ACTIVE.read_text(encoding="utf-8-sig")
        if registered_tools(source) != EXPECTED:
            print("PARADO: ferramentas inesperadas; nada alterado.")
            print(sorted(registered_tools(source)))
            return 2

        candidate = source
        for old, new in OLD_NEW.items():
            # Idempotente: uma segunda execucao nao deve mudar os arquivos.
            if candidate.count(new) == 1 and candidate.count(old) == 0:
                continue
            if candidate.count(old) != 1 or candidate.count(new) != 0:
                print("PARADO: endpoints inesperados:", old, "; nada alterado.")
                return 2
            candidate = candidate.replace(old, new, 1)

        if "wroom_status_texto" in candidate:
            print("PARADO: S3 contem agregacao legada; nada alterado.")
            return 2
        if registered_tools(candidate) != EXPECTED:
            print("PARADO: validacao das ferramentas falhou.")
            return 2

        if candidate == source:
            print("JA ATUALIZADO: IPs estao corretos, nenhuma alteracao necessaria.")
            return 0

        if not local_port_open(8765):
            print("PARADO: MCP nao esta ouvindo na porta 8765; nada alterado.")
            return 2

        print("VALIDANDO REDE...", flush=True)
        print("S3:", verify_board(OLD_NEW["http://192.168.0.7/status"], "BOTIZIN CORE V"), flush=True)
        print("WROOM:", verify_board(OLD_NEW["http://192.168.0.16/status"], "BOTIZIN WROOM V"), flush=True)
    except Exception as exc:
        print("PARADO SEM ALTERACOES:", type(exc).__name__, str(exc)[:220], flush=True)
        return 2

    fd, tempname = tempfile.mkstemp(dir=ROOT, prefix=".servidor-ips-", suffix=".py")
    temp = Path(tempname)
    backup = None
    changed = False
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as f:
            f.write(candidate)
            f.flush()
            os.fsync(f.fileno())
        os.chmod(temp, 0o600)
        py_compile.compile(str(temp), doraise=True)
        smoke_test(temp)

        backup = ROOT / ("servidor.py.backup-ips-" + time.strftime("%Y%m%d-%H%M%S") + "-" + str(os.getpid()))
        shutil.copy2(ACTIVE, backup)
        os.chmod(backup, 0o600)
        print("BACKUP:", backup.name, flush=True)
        os.replace(temp, ACTIVE)
        changed = True
        print("INSTALADO. Reiniciando somente o MCP...", flush=True)
        restart_mcp()
        time.sleep(3)
        if wait_mcp():
            print("MCP ONLINE. CONCLUIDO: 4 ferramentas preservadas; S3 192.168.0.2 e WROOM 192.168.0.10.", flush=True)
            print("NAO e necessario atualizar catalogo do ChatGPT (nomes iguais).", flush=True)
            return 0

        print("FALHA: MCP nao voltou. Restaurando backup...", flush=True)
        shutil.copy2(backup, ACTIVE)
        os.chmod(ACTIVE, 0o600)
        restart_mcp()
        print("BACKUP RESTAURADO.", "MCP online." if wait_mcp() else "Verificar mcp-boot.log.", flush=True)
        return 1
    except Exception as exc:
        print("ERRO:", type(exc).__name__, str(exc)[:220], flush=True)
        if changed and backup is not None and backup.exists():
            shutil.copy2(backup, ACTIVE)
            os.chmod(ACTIVE, 0o600)
            restart_mcp()
            print("BACKUP RESTAURADO; verificar servico MCP.", flush=True)
        return 1
    finally:
        temp.unlink(missing_ok=True)


if __name__ == "__main__":
    sys.exit(main())
