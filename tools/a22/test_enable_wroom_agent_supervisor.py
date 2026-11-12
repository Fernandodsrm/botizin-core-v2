#!/usr/bin/env python3
"""Never launches a process. Test fail-closed supervisor insertion."""
import importlib.util
from pathlib import Path
import unittest

file=Path(__file__).with_name("enable_wroom_agent_supervisor.py")
spec=importlib.util.spec_from_file_location("wroom_supervisor_patcher",file)
mod=importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)
BASE='''from pathlib import Path
import subprocess,time
ROOT=Path("/root/botizin-mcp")
DIAGNOSTIC=ROOT/"diagnostico_supervisor.py"
def main():
        diag_proc=None
        last_diag=0
        now=0
        last_state=0
        mcp=True
        while True:
            if now-last_state>30:
                pass
'''
class PatcherTests(unittest.TestCase):
    def test_idempotence(self):
        updated=mod.patch_source(BASE)
        self.assertIn(mod.MARKER,updated)
        self.assertIn("wroom_module_agent.py",updated)
        self.assertEqual(updated.count("last_wroom=0"),1)
        self.assertIsNone(mod.patch_source(updated))
    def test_fail_closed(self):
        with self.assertRaises(ValueError):
            mod.patch_source('print("unsupported supervisor")\n')
        with self.assertRaises(ValueError):
            mod.patch_source(BASE+BASE)
if __name__=="__main__":
    unittest.main()
