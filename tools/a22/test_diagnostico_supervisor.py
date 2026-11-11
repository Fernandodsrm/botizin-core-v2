#!/usr/bin/env python3
"""Offline regression tests: read-only diagnostic, redaction and MCP injection."""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest import mock
import urllib.error

HERE = Path(__file__).resolve().parent

def load(name, path):
    s = importlib.util.spec_from_file_location(name, HERE / path)
    m = importlib.util.module_from_spec(s)
    s.loader.exec_module(m)
    return m

d = load("botizin_health_test", "diagnostico_supervisor.py")
installer = load("botizin_diag_installer_test", "instalar_diagnostico_mcp.py")


class Tests(unittest.TestCase):
    def test_tunnel_503_is_failure(self):
        err = urllib.error.HTTPError("http://127.0.0.1:18080/readyz", 503, "bad", {}, None)
        with mock.patch.object(d.urllib.request, "build_opener") as opener:
            opener.return_value.open.side_effect = err
            self.assertEqual(d.tunnel_ready(), 503)

    def test_tunnel_not_ready_is_alert_even_with_port(self):
        with mock.patch.object(d, "tunnel_ready", return_value=503), \
             mock.patch.object(d, "port_open", return_value=True), \
             mock.patch.object(d, "board_summary", return_value={"mapa_atual": True,
                      "s3": {"estado": "online"}, "wroom": {"estado": "online"}}), \
             mock.patch.object(d, "read_json", return_value=({}, None)), \
             mock.patch.object(d, "recent", return_value=(True, 0)), \
             mock.patch.object(d, "sanitized_events", return_value=[]):
            status = d.collect()
        self.assertIn("tunel_nao_pronto", status["alertas"])
        self.assertFalse(status["servicos"]["tunel_pronto"])

    def test_map_missing_not_false_offline(self):
        with tempfile.TemporaryDirectory() as folder:
            with mock.patch.object(d, "MAP", Path(folder) / "missing.json"):
                summary = d.board_summary()
        self.assertEqual(summary["s3"]["estado"], "desconhecido")
        self.assertEqual(summary["wroom"]["estado"], "desconhecido")

    def test_stale_map_cannot_report_online(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "dispositivos.json"
            path.write_text(json.dumps({"dispositivos": {
                "s3": {"online": True, "ip": "192.168.0.3", "firmware": "BOTIZIN CORE V0.0.25"},
                "wroom": {"online": False}
            }}))
            with mock.patch.object(d, "MAP", path), mock.patch.object(d, "recent", return_value=(False, 999)):
                result = d.board_summary()
        self.assertEqual(result["s3"]["estado"], "desconhecido_mapa_antigo")

    def test_logs_do_not_expose_secrets(self):
        with tempfile.TemporaryDirectory() as folder:
            log = Path(folder) / "supervisor.log"
            log.write_text(
                "2026-11-10 12:00:00 STATUS: MCP=UP; TUNNEL HTTP=503\n"
                "2026-11-10 12:00:01 CONTROL_PLANE_API_KEY=sk-testsecret\n"
                "2026-11-10 12:00:02 TUNNEL: starting PID 123\n"
            )
            with mock.patch.object(d, "SUPERVISOR_LOG", log):
                events = d.sanitized_events()
        self.assertEqual(len(events), 2)
        self.assertNotIn("sk-testsecret", str(events))

    def test_snapshot_private_atomic(self):
        with tempfile.TemporaryDirectory() as folder:
            with mock.patch.object(d, "OUTPUT", Path(folder) / "diag.json"), \
                 mock.patch.object(d, "ROOT", Path(folder)):
                d.save_snapshot({"ok": True})
                p = Path(folder) / "diag.json"
                self.assertTrue(json.loads(p.read_text())["ok"])
                self.assertEqual(p.stat().st_mode & 0o777, 0o600)

    def test_mcp_patch_preserves_tool_name(self):
        sample = '''
class MockMCP:
    def tool(self):
        return lambda f: f
mcp = MockMCP()
@mcp.tool()
def consultar_status_a22() -> dict:
    """status"""
    return {"origin": "BOTIZIN_A22", "servicos_locais": {}}
if __name__ == "__main__":
    pass
'''
        updated = installer.transform(sample)
        self.assertIn("BOTIZIN_DIAGNOSTICO_SUPERVISOR_V1", updated)
        self.assertIsNone(installer.transform(updated))
        self.assertEqual(updated.count("@mcp.tool()"), 1)
        namespace = {"__name__": "not_main"}
        exec(compile(updated, "test.py", "exec"), namespace)
        info = namespace["consultar_status_a22"]()
        self.assertEqual(info["origin"], "BOTIZIN_A22")
        self.assertIn("diagnostico_supervisor", info)

    def test_mcp_patch_rejects_unexpected_structure(self):
        with self.assertRaises(ValueError):
            installer.transform("def consultar_status_a22():\n    return {}\n")


if __name__ == "__main__":
    unittest.main()
