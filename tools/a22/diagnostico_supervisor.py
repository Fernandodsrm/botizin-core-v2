#!/usr/bin/env python3
"""BOTIZIN A22 read-only health snapshot; never runs a command or changes services.

Executed periodically by the existing Linux supervisor. Only writes its own
private JSON snapshot. Never reads credential files or raw application logs.
"""
import datetime
import json
import os
from pathlib import Path
import re
import shutil
import socket
import tempfile
import time
import urllib.error
import urllib.request

ROOT = Path("/root/botizin-mcp")
OUTPUT = ROOT / "diagnostico-supervisor.json"
SUPERVISOR = ROOT / "supervisor-status.json"
MAP = ROOT / "dispositivos.json"
SUPERVISOR_LOG = ROOT / "supervisor.log"
MAX_BYTES = 65536
FRESH_SECS = 420


def now_utc():
    return datetime.datetime.now(datetime.timezone.utc).isoformat(timespec="seconds")


def read_json(path):
    try:
        if path.stat().st_size > MAX_BYTES:
            return None, "arquivo_grande"
        with path.open("r", encoding="utf-8") as f:
            data = json.load(f)
        if not isinstance(data, dict):
            return None, "formato_invalido"
        return data, None
    except (OSError, UnicodeError, ValueError, TypeError):
        return None, "indisponivel"


def recent(path, seconds=FRESH_SECS):
    try:
        age = max(0, int(time.time() - path.stat().st_mtime))
        return age <= seconds, age
    except OSError:
        return False, None


def port_open(port):
    try:
        with socket.create_connection(("127.0.0.1", port), timeout=0.6):
            return True
    except OSError:
        return False


def tunnel_ready():
    # A 503 is a fault even when the TCP port is accepting connections.
    try:
        with urllib.request.build_opener(urllib.request.ProxyHandler({})).open(
            "http://127.0.0.1:18080/readyz", timeout=2
        ) as r:
            return r.status
    except urllib.error.HTTPError as error:
        return error.code
    except (OSError, urllib.error.URLError, ValueError):
        return 0


def mem_available_mb():
    try:
        with open("/proc/meminfo", encoding="ascii") as f:
            for line in f:
                if line.startswith("MemAvailable:"):
                    return round(int(line.split()[1]) / 1024, 1)
    except (OSError, ValueError, IndexError):
        pass
    return None


def disk_free_gb():
    try:
        return round(shutil.disk_usage(ROOT).free / (1024 ** 3), 2)
    except OSError:
        return None


def sanitized_events():
    # Select known fixed-format supervisor events only. Never forward arbitrary
    # log lines, paths, parameters, environment, command output or credentials.
    allowed = re.compile(
        r"^(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2} )"
        r"(STATUS: MCP=(?:UP|DOWN); TUNNEL HTTP=(?:0|200|503|[1-5]\d\d)"
        r"|MCP: (?:starting PID \d+|stopped verified PID \d+)"
        r"|TUNNEL: (?:starting PID \d+|stopped verified PID \d+)"
        r"|BOTIZIN v2 supervisor active\. No firmware or credentials changed\.)$"
    )
    try:
        # bounded read; use the tail of the file only; no application logs.
        with SUPERVISOR_LOG.open("rb") as f:
            f.seek(0, os.SEEK_END)
            size = f.tell()
            f.seek(max(0, size - 32768))
            text = f.read(32768).decode("utf-8", "replace")
        found = []
        for line in text.splitlines():
            if len(line) <= 180 and allowed.fullmatch(line):
                found.append(line)
        return found[-12:]
    except OSError:
        return []


def board_summary():
    data, error = read_json(MAP)
    fresh, age = recent(MAP)
    out = {"mapa_disponivel": data is not None,
           "mapa_atual": fresh, "mapa_idade_segundos": age}
    if data is None:
        out["erro_mapa"] = error
        out["s3"] = {"estado": "desconhecido"}
        out["wroom"] = {"estado": "desconhecido"}
        return out
    devices = data.get("dispositivos", {})
    if not isinstance(devices, dict):
        devices = {}
    for kind in ("s3", "wroom"):
        item = devices.get(kind, {})
        if not isinstance(item, dict):
            item = {}
        online = item.get("online") is True
        state = "online" if online else "offline"
        if not fresh:
            state = "desconhecido_mapa_antigo"
        ip = item.get("ip") if online else item.get("last_known_ip")
        # IPs are local metadata; never trust arbitrary strings from JSON.
        out[kind] = {"estado": state,
                     "ultimo_ip": ip if isinstance(ip, str) and len(ip) <= 45 else None,
                     "versao": str(item.get("firmware", ""))[:55] if online else None,
                     "wifi": str(item.get("wifi", ""))[:20] if online else None,
                     "led_estado": str(item.get("led_state", ""))[:12] if online and kind == "wroom" else None}
    return out


def collect():
    supervisor, err = read_json(SUPERVISOR)
    state_fresh, age = recent(SUPERVISOR, 120)
    mcp = port_open(8765)
    tunnel = tunnel_ready()
    ssh = port_open(8022)
    macro_port = port_open(8080)
    processes = {}
    if state_fresh and supervisor:
        for kind in ("mcp", "tunnel"):
            value = supervisor.get(kind, {})
            if isinstance(value, dict):
                pidlist = value.get("pids", [])
                processes[kind] = {
                    "quantidade": len(pidlist) if isinstance(pidlist, list) else None,
                    "ultima_recuperacao_ha_segundos": value.get("last_recovery_age_s")
                }
    problems = []
    if not mcp:
        problems.append("mcp_indisponivel")
    if tunnel != 200:
        problems.append("tunel_nao_pronto")
    if not ssh:
        problems.append("ssh_indisponivel")
    if not state_fresh:
        problems.append("supervisor_sem_estado_recente")
    boards = board_summary()
    if boards["mapa_atual"]:
        for kind in ("s3", "wroom"):
            if boards[kind]["estado"] == "offline":
                problems.append(kind + "_offline")
    else:
        problems.append("mapa_esp32_desatualizado")
    try:
        stat_disk = disk_free_gb()
        stat_mem = mem_available_mb()
    except Exception:
        stat_disk = stat_mem = None
    if stat_disk is not None and stat_disk < 1.0:
        problems.append("disco_quase_cheio")
    return {
        "schema": 1,
        "origem": "BOTIZIN_SUPERVISOR_DIAGNOSTICO",
        "coletado_em_utc": now_utc(),
        "saude": "OK" if not problems else "ATENCAO",
        "alertas": problems,
        "servicos": {
            "mcp_porta_8765": mcp,
            "tunel_readyz_http": tunnel,
            "tunel_pronto": tunnel == 200,
            "ssh_porta_8022": ssh,
            "macrodroid_porta_8080": macro_port,
            "macrodroid_identidade_validada": False,
            "tailscale_vpn": "nao_verificado",
        },
        "supervisor": {
            "estado_disponivel": supervisor is not None,
            "estado_recente": state_fresh,
            "estado_idade_segundos": age,
            "processos": processes,
            "eventos_recentes": sanitized_events(),
        },
        "placas": boards,
        "recursos": {
            "memoria_disponivel_mb": stat_mem,
            "disco_livre_gb": stat_disk,
        },
        "limites": (
            "HTTP 200 em /readyz e saude local nao garantem acesso externo. "
            "Portas abertas nao provam identidade do servico. "
            "Falha de energia, VPN e defeitos fisicos exigem verificacao independente."
        ),
    }


def save_snapshot(data):
    ROOT.mkdir(parents=True, exist_ok=True)
    temp = None
    try:
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", dir=ROOT,
                                         prefix=".botizin-diag-", delete=False) as f:
            temp = f.name
            os.chmod(temp, 0o600)
            json.dump(data, f, indent=2, ensure_ascii=False)
            f.write("\n")
            f.flush()
            os.fsync(f.fileno())
        os.replace(temp, OUTPUT)
    finally:
        if temp and os.path.exists(temp):
            os.unlink(temp)


if __name__ == "__main__":
    save_snapshot(collect())
    print("Diagnostico atualizado em", OUTPUT)
