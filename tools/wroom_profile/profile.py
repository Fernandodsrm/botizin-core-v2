#!/usr/bin/env python3
"""Offline WROOM .38 profile build; NEVER publishes, touches OTA, or writes device.
Compares exact commit bea8... compiled under same core, partition table, OLED libs.
"""
import hashlib, json, os, pathlib, re, shutil, subprocess, sys
ROOT=pathlib.Path(__file__).resolve().parents[2]
COMMIT="bea8f4172b01f5bcdc2896bf40c6e8f48ab42549"
FQBN="esp32:esp32:esp32:FlashSize=4M,FlashMode=dio,CPUFreq=240,EraseFlash=none"
EXPECTED=1288144
DIST=ROOT/"wroom-profile-results"
DIST.mkdir(exist_ok=True)
SOURCE=ROOT/"wroom-profile-original"/"firmware"

def cmd(*args):
    subprocess.run(args,check=True)

def replace_exact(s,a,b):
    if s.count(a)!=1:raise RuntimeError("Expected one source anchor: "+a[:90])
    return s.replace(a,b)

def between(s,start,end,replacement):
    if s.count(start)!=1 or s.count(end)!=1:
        raise RuntimeError("missing or ambiguous boundary "+start[:90])
    i=s.index(start);j=s.index(end,i+len(start))
    return s[:i]+replacement+"\n\n"+s[j:]

def profile(src,mode):
    firmware=src/"firmware.ino"
    s=firmware.read_text()
    if mode in ("no_thingsboard","no_thingsboard_and_github"):
        s=between(s,
          "void __attribute__((noinline)) sendTelemetry() {",
          '#include "ping_rpc.h"',
          'void sendTelemetry(){telemetryStatus="DISABLED_FOR_SIZE_TEST";}')
        s=replace_exact(s,'#include "ping_rpc.h"',
          'static uint32_t nextPingPoll=0;\nstatic bool pingStopped=true;\nstatic String pingStatus="DISABLED_FOR_SIZE_TEST";\nstatic void pollPing(){}')
    if mode == "no_thingsboard_and_github":
        s=between(s,
          "void checkInternetOTA(bool manual) {",
          "void installInternetOTA(",
          'void checkInternetOTA(bool manual){(void)manual;internetStatus="OFFLINE_TEST";}')
        s=between(s,
          "void installInternetOTA(",
          "// Separate diagnostic sender:",
          'void installInternetOTA(const String &,const String &,const String &,size_t){}')
        cp=src/"environment_catalog.h"
        cat=cp.read_text()
        cat=between(cat,"static void checkCatalog(){","static bool prepareEnvironment(",
          'static void checkCatalog(){catalogStatus="CACHE_ONLY";}')
        cp.write_text(cat)
    if mode == "no_mdns":
        s=replace_exact(s,'#include "local_name.h"',
          'static bool localNameReady=false;\nstatic void maintainLocalName(){}')
    firmware.write_text(s)

def collect_symbols(elf,outdir):
    tool_dirs=list(pathlib.Path.home().glob(".arduino15/packages/esp32/tools/**/bin/xtensa-esp32-elf-nm"))
    if not tool_dirs:
        print("WARNING: nm binary not found")
        return
    nm=str(tool_dirs[-1])
    r=subprocess.run([nm,"-C","-S","--size-sort",str(elf)],capture_output=True,text=True)
    if r.returncode:print("WARN nm:",r.stderr[:500]);return
    (outdir/"symbols.txt").write_text(r.stdout[-100000:])
    entries=[]
    for line in r.stdout.splitlines():
        parts=line.split(maxsplit=3)
        if len(parts)==4:
            try:
                n=int(parts[1],16)
                if n:entries.append({"symbol":parts[3][:130],"bytes":n})
            except ValueError:pass
    entries.sort(key=lambda x:x["bytes"],reverse=True)
    (outdir/"top-symbols.json").write_text(json.dumps(entries[:120],indent=2))
    print("TOP_SYMBOLS",json.dumps(entries[:25],ensure_ascii=False),flush=True)

def main():
    subprocess.run(["git","cat-file","-e",COMMIT+"^{commit}"],check=True)
    # Freeze exact Git commit from archive; no changes in current firmware source tree.
    SOURCE.mkdir(parents=True,exist_ok=True)
    for name in ["firmware.ino","diagnostics.h","environment_catalog.h","face_engine.h",
      "face_state.h","internet_config.h","local_name.h","module_bridge.h","navigation.h",
      "navigation_model.h","oled_status.h","ota_confirmation.h","partitions.csv",
      "ping_rpc.h","provisioning.h","ps4_config.h","remote_led.h","version.h",
      "catalog_state.h"]:
        blob=subprocess.check_output(["git","show",f"{COMMIT}:wroom/firmware/{name}"])
        (SOURCE/name).write_bytes(blob)
    modes=["baseline","no_thingsboard","no_mdns","no_thingsboard_and_github"]
    output=[]
    for mode in modes:
        trial=ROOT/"wroom-profile-work"/mode/"firmware"
        trial.mkdir(parents=True,exist_ok=True)
        shutil.copytree(SOURCE,trial,dirs_exist_ok=True)
        (trial/"version.h").write_text('#pragma once\n#define BOTIZIN_VERSION "0.0.38"\n')
        if mode!="baseline":profile(trial,mode)
        out=ROOT/"wroom-profile-build"/mode
        out.mkdir(parents=True,exist_ok=True)
        print(f"PROFILE_START {mode}",flush=True)
        proc=subprocess.run(["arduino-cli","compile","--fqbn",FQBN,"--output-dir",str(out),str(trial)],
                            stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
        (DIST/f"{mode}-compile.log").write_text(proc.stdout)
        print("\n".join(x for x in proc.stdout.splitlines() if "Sketch uses" in x or "Global variables" in x)[-1500:],flush=True)
        if proc.returncode:
            print("BUILD ERROR: "+mode+"\n"+proc.stdout[-5000:],flush=True)
            sys.exit(proc.returncode)
        b=(out/"firmware.ino.bin").read_bytes()
        obj={"profile":mode,"bytes":len(b),"sha256":hashlib.sha256(b).hexdigest(),"baseline_delta_bytes":len(b)-EXPECTED}
        if mode=="baseline":
            assert len(b)==EXPECTED, f"Baseline not reproduced: {len(b)} != {EXPECTED}"
        output.append(obj)
        print("PROFILE_RESULT",json.dumps(obj),flush=True)
        collect_symbols(out/"firmware.ino.elf",DIST/mode) if (DIST/mode).mkdir(exist_ok=True) is None else None
    (DIST/"comparison.json").write_text(json.dumps(output,indent=2)+"\n")
    print("COMPARISON",json.dumps(output),flush=True)
if __name__=="__main__":main()
