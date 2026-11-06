#!/usr/bin/env python3
"""Offline smoke tests for BOTIZIN A22 startup chain; no device actions."""
import importlib.util
import io
from pathlib import Path
import unittest
from unittest import mock
import urllib.error

BASE=Path(__file__).resolve().parent
def load(name,filename):
    spec=importlib.util.spec_from_file_location(name,BASE/filename)
    mod=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod
supervisor=load("supervisor_linux","supervisor_linux.py")
mapper=load("mapear_rede","mapear_rede.py")

class Response:
    status=200
    def __init__(self,body=b""):
        self.body=body
    def __enter__(self):return self
    def __exit__(self,*args):return False
    def read(self,size=-1):return self.body[:size]

class Tests(unittest.TestCase):
    def test_tunnel_ready(self):
        with mock.patch.object(supervisor.urllib.request,"urlopen",return_value=Response()):
            self.assertEqual(supervisor.tunnel_http(),200)
    def test_503_is_not_healthy(self):
        failure=urllib.error.HTTPError("http://127.0.0.1:18080/readyz",503,"not ready",{},None)
        with mock.patch.object(supervisor.urllib.request,"urlopen",side_effect=failure):
            self.assertEqual(supervisor.tunnel_http(),503)
    def test_tunnel_port_closed(self):
        with mock.patch.object(supervisor.urllib.request,"urlopen",side_effect=ConnectionRefusedError()):
            self.assertEqual(supervisor.tunnel_http(),0)
    def test_precise_process_matching(self):
        cmds=[(11,[str(supervisor.PYTHON),str(supervisor.SERVER)]),
              (12,[str(supervisor.TUNNEL),"run","--profile","botizin-a22"]),
              (13,[str(supervisor.TUNNEL),"run","--profile","another"]),
              (14,["/usr/bin/python3","not-botizin.py"])]
        with mock.patch.object(supervisor,"running_commands",return_value=cmds):
            self.assertEqual(supervisor.pids("mcp"),[11])
        with mock.patch.object(supervisor,"running_commands",return_value=cmds):
            self.assertEqual(supervisor.pids("tunnel"),[12])
    def test_wroom_verified_by_identity(self):
        data=b"BOTIZIN WROOM V0.0.38\nIP: 192.168.0.3\nWIFI: OK\nLED_STATE: OFF\n"
        with mock.patch.object(mapper.urllib.request,"urlopen",return_value=Response(data)):
            info=mapper.verify("192.168.0.3")
        self.assertEqual(info["kind"],"wroom")
        self.assertEqual(info["led_state"],"OFF")
    def test_reject_spoofed_status(self):
        data=b"BOTIZIN WROOM V0.0.38\nIP: 192.168.0.99\nWIFI: OK\n"
        with mock.patch.object(mapper.urllib.request,"urlopen",return_value=Response(data)):
            self.assertIsNone(mapper.verify("192.168.0.3"))
    def test_reject_external_scan(self):
        with mock.patch.object(mapper.urllib.request,"urlopen") as open_mock:
            self.assertIsNone(mapper.verify("100.105.142.106"))
            open_mock.assert_not_called()

if __name__=="__main__":unittest.main()
