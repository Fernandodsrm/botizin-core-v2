#!/usr/bin/env python3
"""Adds a read-only supervisor summary to existing consultar_status_a22 MCP tool.

Preserves every existing tool. Does not stop/restart any process.
Will abort on unexpected structure; creates a private backup before atomic change.
"""
import ast
import datetime as dt
import os
from pathlib import Path
import shutil
import tempfile

TARGET = Path("/root/botizin-mcp/servidor.py")
MARKER = "# BOTIZIN_DIAGNOSTICO_SUPERVISOR_V1"
ADDITION = '''
    # BOTIZIN_DIAGNOSTICO_SUPERVISOR_V1
    try:
        import json as _botizin_json
        import time as _botizin_time
        from pathlib import Path as _botizin_Path
        _p = _botizin_Path("/root/botizin-mcp/diagnostico-supervisor.json")
        if _p.stat().st_size > 65536:
            _resumo = {"estado": "arquivo_grande"}
        elif not 0 <= _botizin_time.time() - _p.stat().st_mtime <= 240:
            _resumo = {"estado": "desatualizado"}
        else:
            with _p.open("r", encoding="utf-8") as _file:
                _resumo = _botizin_json.load(_file)
            if not isinstance(_resumo, dict) or _resumo.get("origem") != "BOTIZIN_SUPERVISOR_DIAGNOSTICO":
                _resumo = {"estado": "formato_invalido"}
        resultado["diagnostico_supervisor"] = _resumo
    except (OSError, ValueError, TypeError, UnicodeError):
        resultado["diagnostico_supervisor"] = {"estado": "ainda_nao_coletado"}
    return resultado
'''

def is_tool(node):
    if not isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
        return False
    for dec in node.decorator_list:
        f = dec.func if isinstance(dec, ast.Call) else dec
        if isinstance(f, ast.Attribute) and isinstance(f.value, ast.Name):
            if f.value.id == "mcp" and f.attr == "tool":
                return True
    return False

def transform(source):
    if MARKER in source:
        return None
    tree = ast.parse(source)
    tools = [x for x in tree.body if is_tool(x)]
    target = [x for x in tools if x.name == "consultar_status_a22"]
    if len(target) != 1:
        raise ValueError("Expected one existing consultar_status_a22 MCP tool")
    function = target[0]
    if not function.body or not isinstance(function.body[-1], ast.Return):
        raise ValueError("Status A22 function does not end in direct return")
    ret = function.body[-1]
    if not isinstance(ret.value, ast.Dict):
        raise ValueError("Status A22 return format unexpectedly changed")
    lines = source.splitlines(keepends=True)
    line = lines[ret.lineno - 1]
    if not line.strip().startswith("return {"):
        raise ValueError("Status A22 return indentation unexpected")
    indent = line[:len(line)-len(line.lstrip())]
    if indent != "    ":
        raise ValueError("Unexpected A22 function indentation")
    lines[ret.lineno - 1] = line.replace("return {", "resultado = {", 1)
    lines[ret.end_lineno:ret.end_lineno] = [ADDITION]
    candidate = "".join(lines)
    compile(candidate, str(TARGET), "exec")
    again = ast.parse(candidate)
    new_tools = [x.name for x in again.body if is_tool(x)]
    if new_tools != [x.name for x in tools]:
        raise ValueError("MCP tool list changed")
    if candidate.count(MARKER) != 1:
        raise ValueError("Diagnostic marker invalid")
    return candidate

def main():
    if not TARGET.is_file():
        raise SystemExit("STOP: servidor.py missing")
    source = TARGET.read_text(encoding="utf-8-sig")
    candidate = transform(source)
    if candidate is None:
        print("JA INSTALADO: consulta A22 ja incorpora diagnostico; sem mudancas.")
        return
    backup = TARGET.with_name(
        "servidor.py.bak-diag-" + dt.datetime.now(dt.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    )
    shutil.copy2(TARGET, backup)
    os.chmod(backup, 0o600)
    temp = None
    try:
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", dir=TARGET.parent,
                                         prefix=".diag-mcp-", delete=False) as out:
            temp = out.name
            os.chmod(temp, 0o600)
            out.write(candidate)
            out.flush()
            os.fsync(out.fileno())
        os.replace(temp, TARGET)
    finally:
        if temp and os.path.exists(temp):
            os.unlink(temp)
    print("INSTALADO: consultar_status_a22 com diagnostico_supervisor.")
    print("BACKUP PRIVADO:", backup)
    print("SERVIDOR NAO REINICIADO (instalador externo cuida disso).")

if __name__ == "__main__":
    main()
