#!/usr/bin/env python3
"""CI smoke test: really start the isolated MCP on Windows and list its tools.
No ESP32 calls, no credentials, no LAN exposure.
"""
import asyncio
import socket
import subprocess
import sys
import time
from pathlib import Path


async def check_mcp():
    from mcp import Client
    async with Client("http://127.0.0.1:8766/mcp") as c:
        result = await c.list_tools()
        names = {t.name for t in result.tools}
        expected = {
            "consultar_status_botizin", "consultar_telemetria_s3",
            "consultar_status_wroom", "controlar_led_wroom",
            "consultar_central_windows",
        }
        assert expected.issubset(names), sorted(names)
        r = await c.call_tool("consultar_central_windows", {})
        assert not r.is_error
        print("WINDOWS_MCP_REAL_PROTOCOL_OK", sorted(names))


def main():
    path = Path(__file__).with_name("windows_mcp.py")
    p = subprocess.Popen(
        [sys.executable, "-u", str(path)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    try:
        for _ in range(80):
            if p.poll() is not None:
                raise RuntimeError("MCP exited before health test")
            try:
                with socket.create_connection(("127.0.0.1", 8766), timeout=0.2):
                    break
            except OSError:
                time.sleep(0.25)
        else:
            raise TimeoutError("MCP port not listening")
        asyncio.run(check_mcp())
    finally:
        p.terminate()
        try:
            out, err = p.communicate(timeout=8)
        except subprocess.TimeoutExpired:
            p.kill()
            out, err = p.communicate()
        if p.returncode and not out:
            print(err.decode("utf-8", "replace")[-1500:])


if __name__ == "__main__":
    main()
