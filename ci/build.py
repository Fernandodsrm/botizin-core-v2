"""Cloud compile only. Publishing remains a separate, explicitly requested step."""
import hashlib
import json
import os
import pathlib
import re
import shutil
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
FQBN = "esp32:esp32:esp32s3:FlashSize=16M,FlashMode=qio,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB,CPUFreq=240,USBMode=hwcdc,CDCOnBoot=cdc,EraseFlash=none"


def main():
    source = ROOT / "firmware"
    m = re.search(r'^#define BOTIZIN_VERSION "(\d+\.\d+\.\d+)"$',
                  (source / "version.h").read_text(), re.MULTILINE)
    if not m:
        raise SystemExit("Invalid firmware version. Nothing published.")
    version = m.group(1)
    sketch = (source / "firmware.ino").read_text()
    if (source / "secrets.h").exists() or '#include "secrets.h"' in sketch:
        raise SystemExit("No compiled Wi-Fi credentials allowed in cloud/public builds.")
    if "static uint8_t buffer[4096]" not in sketch:
        raise SystemExit("Validated stack fix is missing. STOP.")
    dist = ROOT / "dist"
    dist.mkdir(exist_ok=True)
    cores = subprocess.check_output(["arduino-cli", "core", "list", "--format", "json"], text=True)
    (dist / "core-list.json").write_text(cores)
    if not any(c.get("id") == "esp32:esp32" and c.get("installed_version") == "3.3.12"
               for c in json.loads(cores).get("platforms", [])):
        raise SystemExit("Required core 3.3.12 is not installed. STOP.")
    out = ROOT / "compiler-output"
    out.mkdir(exist_ok=True)
    command = ["arduino-cli", "compile", "--fqbn", FQBN, "--output-dir", str(out), str(source)]
    print("BUILD", version, "SOURCE_COMMIT", os.environ.get("GITHUB_SHA", "local"), flush=True)
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                               text=True, bufsize=1)
    with (dist / "compile.log").open("w") as log:
        for line in process.stdout:
            print(line, end="", flush=True)
            log.write(line)
            log.flush()
    if process.wait():
        raise SystemExit("Compile failed. No release or manifest publication.")
    binary = out / "firmware.ino.bin"
    payload = binary.read_bytes()
    if not payload or payload[0] != 0xE9 or len(payload) > 3145728:
        raise SystemExit("Invalid/oversized ESP32 image. No publication.")
    if ("BOTIZIN CORE V").encode() not in payload or version.encode() not in payload:
        raise SystemExit("Firmware identity/version absent. No publication.")
    if version == "0.0.8" and "Deu certo na atualização".encode() not in payload:
        raise SystemExit("0.0.8 test message absent. No publication.")
    sha = hashlib.sha256(payload).hexdigest()
    shutil.copy2(binary, dist / "firmware.bin")
    (dist / "firmware.bin.sha256").write_text(sha + "\n")
    elf = out / "firmware.ino.elf"
    if elf.exists():
        shutil.copy2(elf, dist / "firmware.elf")
    manifest = {"board": "esp32s3-n16r8", "version": version, "size": len(payload),
                "sha256": sha,
                "url": "https://raw.githubusercontent.com/Fernandodsrm/botizin-core-v2/main/releases/" + version + "/firmware.bin"}
    (dist / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    record = {"version": version, "core": "3.3.12", "fqbn": FQBN,
              "source_commit": os.environ.get("GITHUB_SHA", "local"),
              "run_id": os.environ.get("GITHUB_RUN_ID", "local"),
              "bytes": len(payload), "sha256": sha, "credentials_compiled": False,
              "published": False}
    (dist / "build-record.json").write_text(json.dumps(record, indent=2) + "\n")
    print(json.dumps(record, indent=2), flush=True)
    print("CLOUD COMPILE VERIFIED. No OTA publication. Physical test still required.", flush=True)


if __name__ == "__main__":
    main()
