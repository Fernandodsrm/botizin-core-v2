#!/usr/bin/env python3
"""BOTIZIN Windows HUB provisório: MCP de loopback, sem A22 e sem túnel público.

Não faz OTA, não executa shell remoto, não lê segredos, não controla o Android.
Requer mcp Python SDK v2 (MCPServer) em venv próprio.
"""
import argparse
import ipaddress
import json
import re
import sys
import urllib.error
import urllib.request
from typing import Any

S3 = "192.168.0.24"
WROOM = "192.168.0.23"
LOCAL_PORT = 8766
TRUSTED_LAN = ipaddress.ip_network("192.168.0.0/24")
NO_PROXY = urllib.request.build_opener(urllib.request.ProxyHandler({}))
WROOM_VERSIONS = ("0.0.40", "0.0.41", "0.0.42")


def http_get(url: str, size: int = 13000, timeout: float = 4) -> bytes:
    with NO_PROXY.open(url, timeout=timeout) as response:
        if response.status != 200:
            raise RuntimeError("HTTP_NOT_200")
        data = response.read(size + 1)
        if len(data) > size:
            raise RuntimeError("RESPONSE_TOO_LARGE")
        return data


def checked_ip(address: str) -> str:
    addr = ipaddress.ip_address(address)
    if not isinstance(addr, ipaddress.IPv4Address) or addr not in TRUSTED_LAN:
        raise ValueError("IP_NOT_IN_LAN")
    return str(addr)


def get_s3() -> dict[str, Any]:
    payload = json.loads(http_get(f"http://{checked_ip(S3)}/status", 1500))
    if not isinstance(payload, dict) or payload.get("origin") != "XIAOZHI_REAL":
        raise ValueError("S3_IDENTITY_MISMATCH")
    return payload


def get_wroom_raw() -> str:
    s = http_get(f"http://{checked_ip(WROOM)}/status", 13000).decode("utf-8", "replace")
    if not any(s.startswith(f"BOTIZIN WROOM V{v}\n") for v in WROOM_VERSIONS):
        raise ValueError("WROOM_IDENTITY_MISMATCH")
    if f"\nIP: {WROOM}\n" not in s or "\nWIFI: OK\n" not in s:
        raise ValueError("WROOM_LAN_MISMATCH")
    return s


def wroom_fields(s: str) -> dict[str, str]:
    allow = {
        "LED_STATE", "LED_GPIO", "OLED", "OLED_PAGE", "FACE_FRAMES",
        "FACE_MAX_GAP_MS", "RUNNING_PARTITION", "BOOT_PARTITION",
        "NEXT_UPDATE_PARTITION", "RUNNING_OTA_STATE", "DIAG_FREE",
        "DIAG_MIN", "DIAG_OTA_FREE", "VOICE_SYNC_MODE",
        "VOICE_SYNC_EVENTS", "VOICE_SYNC_AGE_MS", "VOICE_LAB_RETURN_BASE_ARMED",
        "RESET_REASON", "UPTIME_SECONDS", "OTA",
    }
    result: dict[str, str] = {}
    for line in s.splitlines():
        key, sep, value = line.partition(": ")
        if sep and key in allow:
            result[key.lower()] = value[:150]
    result["firmware"] = s.splitlines()[0][:80]
    result["ip"] = WROOM
    return result


def consultar_status_botizin() -> dict:
    """Consulta o estado REAL da S3 (XiaoZhi) sem retornar activation_code."""
    try:
        d = get_s3()
        return {
            "ok": True, "origem": "XIAOZHI_REAL",
            "state": d.get("state"),
            "web_audio_connected": d.get("web_audio_connected"),
            "mic_pcm_frames": d.get("mic_pcm_frames"),
            "speaker_pcm_frames": d.get("speaker_pcm_frames"),
            "speaker_pcm_dropped_samples": d.get("speaker_pcm_dropped_samples"),
            "ai_backend": d.get("ai_backend"),
            "lab_confirmed": d.get("lab_confirmed"),
            "ip": S3,
        }
    except Exception as e:
        return {"ok": False, "origem": "S3", "erro": type(e).__name__, "detalhe": str(e)[:100]}


def consultar_telemetria_s3() -> dict:
    """Lê contadores técnicos da S3, sem reiniciá-la."""
    try:
        payload = json.loads(http_get(f"http://{checked_ip(S3)}/lab/telemetry", 5000))
        if payload.get("lab") != "0.0.41":
            raise ValueError("LAB_VERSION_MISMATCH")
        allow = [
            "lab", "uptime_ms", "xiaozhi_state", "ws_rx_errors", "ws_tx_errors",
            "ws_open_count", "mic_codec_reads_failed", "mic_browser_rejected",
            "speaker_pcm_queued_samples", "speaker_pcm_dropped_samples",
            "speaker_popped_samples", "backend_opus_rx_frames",
            "backend_opus_rx_accepted", "mcp_probe_count",
        ]
        return {"ok": True, **{k: payload.get(k) for k in allow}}
    except Exception as e:
        return {"ok": False, "erro": type(e).__name__, "detalhe": str(e)[:100]}


def consultar_status_wroom() -> dict:
    """Consulta status REAL da WROOM, OLED, OTA e contador de olhos."""
    try:
        return {"ok": True, **wroom_fields(get_wroom_raw())}
    except Exception as e:
        return {"ok": False, "origem": "WROOM", "erro": type(e).__name__,
                "detalhe": str(e)[:100]}


def controlar_led_wroom(estado: str) -> dict:
    """Liga/desliga LED GPIO26 da WROOM por até 30s, sem alterar firmware."""
    if estado not in ("ligar", "desligar"):
        return {"ok": False, "erro": "Use 'ligar' ou 'desligar'"}
    try:
        before = get_wroom_raw()
        if "\nLED_GPIO: 26\n" not in before:
            raise ValueError("EXPECTED_LED_NOT_PRESENT")
        enabled = "1" if estado == "ligar" else "0"
        url = f"http://{checked_ip(WROOM)}/led?enabled={enabled}"
        req = urllib.request.Request(url, data=b"", method="POST")
        with NO_PROXY.open(req, timeout=5) as r:
            reply = r.read(256).decode("utf-8", "replace")
            if r.status != 200:
                raise RuntimeError("LED_RESPONSE_BAD")
        expected = f"LED_STATE: {'ON' if enabled=='1' else 'OFF'}"
        after = get_wroom_raw()
        ok = expected in reply and f"\n{expected}\n" in after
        return {"ok": ok, "origem": "WROOM_REAL", "gpio": 26,
                "estado": "ON" if enabled=="1" else "OFF",
                "auto_off_segundos": 30 if enabled=="1" else 0,
                "confirmacao": "status medido" if ok else "sem confirmacao"}
    except Exception as e:
        return {"ok": False, "erro": type(e).__name__, "detalhe": str(e)[:100]}


def consultar_central_windows() -> dict:
    """Estado da central provisória: apenas comunicação local, sem A22."""
    return {"central": "BOTIZIN_WINDOWS_PROVISORIO", "mcp_local": True,
            "porta_local": LOCAL_PORT, "bind": "127.0.0.1",
            "s3_ip": S3, "wroom_ip": WROOM,
            "a22": "OFFLINE_NAO_ALTERADO",
            "tunnel_externo": "NAO_CONFIGURADO",
            "controle_fisico": "LED_WROOM_SE_AUTORIZADO"}


def self_test() -> None:
    assert checked_ip(S3) == S3
    for bad in ("127.0.0.1", "192.168.1.4", "8.8.8.8"):
        try:
            checked_ip(bad)
            raise AssertionError("address allowed: " + bad)
        except ValueError:
            pass
    example = ("BOTIZIN WROOM V0.0.40\nIP: 192.168.0.23\n"
               "WIFI: OK\nLED_STATE: OFF\nOLED: READY\n"
               "VOICE_SYNC_EVENTS: 4\nLAST_PERSISTED_OTA_ATTEMPT:\n"
               "SECRET: not-to-show\n")
    d = wroom_fields(example)
    assert d["voice_sync_events"] == "4"
    assert "SECRET" not in str(d)
    assert consultar_central_windows()["porta_local"] == 8766
    print("WINDOWS_HUB_SELF_TEST_OK")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", type=int, default=LOCAL_PORT)
    ap.add_argument("--self-test", action="store_true")
    args = ap.parse_args()
    if args.self_test:
        self_test()
        return
    if args.port != LOCAL_PORT:
        raise SystemExit("STOP: this temporary server uses 127.0.0.1:8766 only.")
    # SDK v2 is intentionally isolated from the user's original MCP 8765.
    from mcp.server import MCPServer
    mcp = MCPServer("BOTIZIN-Windows-Temporario")
    for fn in (consultar_status_botizin, consultar_telemetria_s3,
               consultar_status_wroom, controlar_led_wroom,
               consultar_central_windows):
        mcp.tool()(fn)
    print("BOTIZIN_WINDOWS_MCP: http://127.0.0.1:8766/mcp LOCAL ONLY", flush=True)
    mcp.run(transport="streamable-http", host="127.0.0.1", port=LOCAL_PORT,
            streamable_http_path="/mcp", stateless_http=True,
            json_response=True)


if __name__ == "__main__":
    main()
