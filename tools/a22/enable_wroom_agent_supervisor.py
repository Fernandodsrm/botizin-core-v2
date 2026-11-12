#!/usr/bin/env python3
"""Patch only the existing A22 Linux supervisor to poll WROOM module requests.

Does not start, stop or signal any process itself. Creates 0600 backup;
idempotent and fail-closed on unexpected supervisor source.
"""
from pathlib import Path
import datetime
import os
import shutil
import tempfile

ROOT=Path("/root/botizin-mcp")
SUP=ROOT/"botizin-supervisor.py"
AGENT=ROOT/"wroom_module_agent.py"
MARKER="# BOTIZIN_WROOM_AGENT_040"


def patch_source(source):
    if MARKER in source:
        return None
    anchors=[
        ('DIAGNOSTIC=ROOT/"diagnostico_supervisor.py"\n',
         'DIAGNOSTIC=ROOT/"diagnostico_supervisor.py"\n'
         'WROOM_AGENT=ROOT/"wroom_module_agent.py"  # BOTIZIN_WROOM_AGENT_040\n'
         'WROOM_INTERVAL=12\n'),
        ('        diag_proc=None\n        last_diag=0\n',
         '        diag_proc=None\n        last_diag=0\n'
         '        wroom_proc=None\n        last_wroom=0\n'),
        ('            if now-last_state>30:\n',
         '''            # Hardware upload is initiated ONLY after physical confirmation
            # on WROOM; A22 checks the official GitHub hash and keeps a local cache.
            # A bounded one-shot process; never blocks MCP/tunnel health checks.
            if WROOM_AGENT.is_file() and mcp and now-last_wroom>WROOM_INTERVAL:
                if wroom_proc is None or wroom_proc.poll() is not None:
                    with (ROOT/"wroom-module-agent.log").open("ab") as out:
                        wroom_proc=subprocess.Popen(["/usr/bin/python3",str(WROOM_AGENT)],
                            cwd=ROOT,stdin=subprocess.DEVNULL,
                            stdout=out,stderr=subprocess.STDOUT,start_new_session=True)
                    last_wroom=now

            if now-last_state>30:
'''),
    ]
    updated=source
    for original,replacement in anchors:
        if updated.count(original)!=1:
            raise ValueError("Unexpected supervisor source, refusing patch: "+original[:65])
        updated=updated.replace(original,replacement)
    compile(updated,str(SUP),"exec")
    assert updated.count(MARKER)==1
    return updated


def main():
    if not SUP.is_file() or not AGENT.is_file():
        raise SystemExit("STOP: supervisor or agent is absent")
    source=SUP.read_text(encoding="utf-8")
    candidate=patch_source(source)
    if candidate is None:
        print("ALREADY_PATCHED: no change")
        return
    compile(AGENT.read_text(encoding="utf-8"),str(AGENT),"exec")
    if "BOTIZIN v2 supervisor active" not in source:
        raise SystemExit("STOP: unsupported supervisor version")
    now=datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    backup=SUP.with_name(SUP.name+".bak-wroom040-"+now)
    shutil.copy2(SUP,backup)
    os.chmod(backup,0o600)
    temp=None
    try:
        with tempfile.NamedTemporaryFile("w",dir=SUP.parent,prefix=".supervisor-wroom-",encoding="utf-8",delete=False) as out:
            temp=out.name
            os.chmod(temp,0o600)
            out.write(candidate)
            out.flush();os.fsync(out.fileno())
        os.replace(temp,SUP)
    finally:
        if temp and os.path.exists(temp):os.unlink(temp)
    print("PATCHED_SUPERVISOR: agent polls once every 12s in background")
    print("BACKUP:",backup)
    print("NO PROCESS RESTARTED BY THIS SCRIPT")


if __name__=="__main__":
    main()
