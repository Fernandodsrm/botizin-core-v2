#!/usr/bin/env python3
"""BOTIZIN: instala a terceira ferramenta MCP de consulta sem alterar firmwares.

Usa como base a copia comprovada contendo apenas as duas ferramentas
independentes (S3 e WROOM); preserva todos os parametros de inicializacao.
Nao gerencia credenciais, nem edita manifestos OTA.
"""

from __future__ import annotations

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
BASELINE = ROOT / "servidor-antes-duas.py"
PATTERN = r"^/root/botizin-mcp/\.venv/bin/python servidor\.py$"
TOOLS_EXPECTED = {"consultar_status_botizin", "consultar_status_wroom"}
THIRD_NAME = "consultar_status_ambas"

THIRD_TOOL = '''@mcp.tool()
def consultar_status_ambas() -> dict:
    """Consulta S3 e WROOM em uma unica chamada, com respostas separadas."""
    def consultar(nome, funcao):
        try:
            resultado = funcao()
            texto = resultado.get("status_texto", "")
            esperado = "BOTIZIN CORE" if nome == "S3" else "BOTIZIN WROOM"
            if not isinstance(texto, str) or not texto.startswith(esperado):
                return {"ok": False, "erro": "RESPOSTA_INESPERADA"}
            return {"ok": True, "origin": resultado.get("origin", nome), "status_texto": texto}
        except Exception as exc:
            return {"ok": False, "erro": type(exc).__name__}

    return {
        "origin": "BOTIZIN_DUAS_PLACAS",
        "s3": consultar("S3", consultar_status_botizin),
        "wroom": consultar("WROOM", consultar_status_wroom),
    }


'''


def registered_tools(source: str) -> set[str]:
    parsed = ast.parse(source)
    result: set[str] = set()
    for node in parsed.body:
        if not isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
            continue
        for decorator in node.decorator_list:
            if isinstance(decorator, ast.Call) and isinstance(decorator.func, ast.Attribute):
                d = decorator.func
                if isinstance(d.value, ast.Name) and d.value.id == "mcp" and d.attr == "tool":
                    result.add(node.name)
    return result


def just_s3(response: dict) -> bool:
    return (isinstance(response, dict) and "wroom_status_texto" not in response
            and str(response.get("status_texto", "")).startswith("BOTIZIN CORE V"))


def test(candidate: Path) -> None:
    import runpy
    funcs = runpy.run_path(str(candidate))
    assert registered_tools(candidate.read_text()) == (TOOLS_EXPECTED | {THIRD_NAME})
    s3 = funcs["consultar_status_botizin"]()
    assert just_s3(s3), "S3 ainda mistura a resposta da WROOM"
    wroom = funcs["consultar_status_wroom"]()
    assert str(wroom.get("status_texto", "")).startswith("BOTIZIN WROOM V")
    combined = funcs[THIRD_NAME]()
    assert combined.get("origin") == "BOTIZIN_DUAS_PLACAS"
    assert combined["s3"].get("ok") is True
    assert combined["wroom"].get("ok") is True
    print("TESTES OK: S3 exclusiva; WROOM exclusiva; terceira consulta independente.", flush=True)
    print("S3:", s3["status_texto"].splitlines()[0], flush=True)
    print("WROOM:", wroom["status_texto"].splitlines()[0], flush=True)


def is_listening() -> bool:
    try:
        with socket.create_connection(("127.0.0.1", 8765), timeout=1.5):
            return True
    except OSError:
        return False


def wait_for_mcp(seconds: float = 40.0) -> bool:
    end = time.monotonic() + seconds
    while time.monotonic() < end:
        if is_listening():
            return True
        time.sleep(2)
    return False


def signal_mcp() -> None:
    subprocess.run(["pkill", "-f", PATTERN], check=False)


def main() -> int:
    if not ACTIVE.is_file() or not BASELINE.is_file():
        print("PARADO: nao encontrei servidor.py e servidor-antes-duas.py. Nada alterado.")
        return 2
    active_source = ACTIVE.read_text()
    active_tools = registered_tools(active_source)
    if active_tools == (TOOLS_EXPECTED | {THIRD_NAME}) and "wroom_status_texto" not in active_source:
        print("JA INSTALADO: as tres ferramentas independentes ja existem.")
        return 0
    if active_tools != TOOLS_EXPECTED:
        print("PARADO: conjunto de ferramentas atual diferente do esperado; nada alterado.")
        print("Ferramentas:", ", ".join(sorted(active_tools)))
        return 2
    base_source = BASELINE.read_text()
    if registered_tools(base_source) != TOOLS_EXPECTED or "wroom_status_texto" in base_source:
        print("PARADO: backup de duas ferramentas nao corresponde a base limpa. Nada alterado.")
        return 2
    main_pattern = r'(?m)^if __name__\s*==\s*[\'\"]__main__[\'\"]\s*:'
    matches = list(re.finditer(main_pattern, base_source))
    if len(matches) != 1:
        print("PARADO: bloco de inicializacao inesperado. Nada alterado.")
        return 2
    # A unica alteracao de codigo: acrescentar terceira funcao ANTES do startup.
    candidate_text = (base_source[: matches[0].start()].rstrip() + "\n\n\n"
                      + THIRD_TOOL + base_source[matches[0].start():])
    if registered_tools(candidate_text) != (TOOLS_EXPECTED | {THIRD_NAME}):
        print("PARADO: registro da terceira ferramenta invalido. Nada alterado.")
        return 2
    if "wroom_status_texto" in candidate_text:
        print("PARADO: consulta S3 ainda agregada. Nada alterado.")
        return 2
    fd, tmp = tempfile.mkstemp(prefix=".servidor-commandcenter-", suffix=".py", dir=ROOT)
    tmp_path = Path(tmp)
    backup = None
    try:
        with os.fdopen(fd, "w") as f:
            f.write(candidate_text)
            f.flush()
            os.fsync(f.fileno())
        os.chmod(tmp_path, 0o600)
        py_compile.compile(str(tmp_path), doraise=True)
        print("VALIDACAO OK. Testando tres consultas reais...", flush=True)
        test(tmp_path)
        # So instalar se a porta MCP estava ativa antes da troca.
        if not is_listening():
            print("PARADO: servidor MCP nao esta ouvindo em 8765; nada alterado.")
            return 2
        backup = ROOT / ("servidor.py.backup-ambas-" + time.strftime("%Y%m%d-%H%M%S"))
        shutil.copy2(ACTIVE, backup)
        os.chmod(backup, 0o600)
        print("BACKUP:", backup.name, flush=True)
        os.replace(tmp_path, ACTIVE)
        tmp_path = None
        print("ARQUIVO INSTALADO. Reiniciando somente o Python MCP...", flush=True)
        signal_mcp()
        # O supervisor existente relanca /root/botizin-mcp/.venv/bin/python servidor.py.
        time.sleep(2)
        if wait_for_mcp(40):
            print("MCP ONLINE (porta 8765).", flush=True)
            print("CONCLUIDO: S3 isolada, WROOM isolada e consultar_status_ambas registrada.", flush=True)
            print("AGORA: ChatGPT > Plugins > BOTIZIN S3 PRIVADO > Atualizar ferramentas.", flush=True)
            return 0
        print("FALHA: MCP nao voltou. Restaurando backup...", flush=True)
        shutil.copy2(backup, ACTIVE)
        os.chmod(ACTIVE, 0o600)
        signal_mcp()
        if wait_for_mcp(40):
            print("BACKUP RESTAURADO: MCP online. Nova versao NAO instalada.", flush=True)
        else:
            print("ALERTA: backup restaurado, mas MCP ainda sem resposta; verificar mcp-boot.log.", flush=True)
        return 1
    except Exception as exc:
        print("ERRO:", type(exc).__name__, str(exc)[:250], flush=True)
        if backup is not None and backup.exists() and tmp_path is None:
            shutil.copy2(backup, ACTIVE)
            os.chmod(ACTIVE, 0o600)
            signal_mcp()
            print("BACKUP RESTAURADO apos erro.", flush=True)
        return 1
    finally:
        if tmp_path is not None:
            tmp_path.unlink(missing_ok=True)


if __name__ == "__main__":
    sys.exit(main())
