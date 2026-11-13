#!/usr/bin/env python3
"""Safe physical WROOM 0.0.43 OLED direct-S3 face pilot.

Only the WROOM ESP32 (specific MAC) is accepted. Preserves app0/base, NVS,
partition table, bootloader. Saves an entire 4 MiB offline recovery backup,
writes the tested LAB image to app1 and verifies flash before switching OTA.
The LAB itself arms app0 for its next reboot. No credentials are uploaded.
"""
from __future__ import annotations
import argparse
import binascii
import datetime as dt
import hashlib
import json
import pathlib
import re
import struct
import subprocess
import sys
import time
import urllib.request
import zipfile

MAC = "30:76:f5:90:67:e0"
IP = "192.168.0.23"
VERSION = "0.0.43"
EXPECTED_BIN_SIZE = 1104848
EXPECTED_BIN_SHA = "ddeb66420f3cb85873938db54f885254bcd364d8d5b1a7c70542b1b60ead4b14"
EXPECTED_SOURCE = "e5c29981a5b49af613cbf109b9e0bdd8eb6fe022"
FLASH_SIZE = 0x400000
APP0 = 0x10000
APP1 = 0x150000
APP_SIZE = 0x140000
OTADATA = 0xe000

def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def stop(why: str):
    raise SystemExit("STOP SEM CONTINUAR: " + why)

def esptool(port: str, *args: str) -> str:
    cmd = [sys.executable, "-m", "esptool", "--chip", "esp32",
           "--port", port, "--baud", "230400", *args]
    done = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                          encoding="utf-8", errors="replace")
    print(done.stdout[-3500:], flush=True)
    if done.returncode:
        stop("Falhou comando USB: " + " ".join(args[:2]))
    return done.stdout

def status(timeout=3):
    # No credentials or privileged endpoints: identity check over local LAN.
    opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
    with opener.open(f"http://{IP}/status", timeout=timeout) as resp:
        if resp.status != 200:
            stop("WROOM /status retornou outro HTTP")
        return resp.read(16000).decode("utf-8", "replace")

def check_old_status():
    s = status()
    needed = ("BOTIZIN WROOM V0.0.40\n",
              f"IP: {IP}\n", "WIFI: OK\n",
              "RUNNING_PARTITION: app0 / ota_0 / address=0x00010000 / size=1310720",
              "BOOT_PARTITION: app0 / ota_0 / address=0x00010000")
    if not all(v in s for v in needed):
        stop("A WROOM deve estar online na base 0.0.40/app0; sem alteracoes.")
    print("WROOM 0.0.40 na OTA0 confirmada pelo HTTP.")

def check_serial(port):
    from serial.tools import list_ports
    available = {p.device.upper():p for p in list_ports.comports()}
    if port.upper() not in available:
        stop(f"Porta {port} ausente. Conecte a WROOM ao USB. Portas: "
             + ", ".join(available))
    # Never write to any other ESP32, especially the S3.
    info = esptool(port, "flash-id")
    if MAC not in info.lower() or not re.search(r"(?:4mb|4 mb)", info.lower()):
        stop("Identidade WROOM/MAC ou flash 4 MB nao confere. Placa nao sera gravada.")
    print("WROOM fisica confirmada pela MAC e flash 4MB.")

def parse_partition_table(backup):
    table = backup[0x8000:0x9000]
    found = {}
    for n in range(0, len(table)-31, 32):
        if table[n:n+2] != b"\xaa\x50":
            continue
        typ,sub,offset,size = struct.unpack_from("<BBII", table, n+2)
        found[typ,sub] = (offset,size)
    expected = {
        (0,16):(APP0,APP_SIZE),
        (0,17):(APP1,APP_SIZE),
        (1,0):(OTADATA,0x2000)
    }
    if any(found.get(key) != value for key,value in expected.items()):
        stop("Tabela de particoes diferente do esperado; OTA1 nao sera tocada.")
    if b"BOTIZIN WROOM V0.0.40" not in backup[APP0:APP0+APP_SIZE]:
        stop("Nao encontrei firmware BOTIZIN no backup APP0.")
    print("Particoes WROOM verificadas: app0, app1, otadata.")

def package_from_zip(zip_path):
    with zipfile.ZipFile(zip_path) as z:
        filename = "0.0.43/firmware.ino.bin"
        recordname = "0.0.43/build-record.json"
        try:
            blob = z.read(filename)
            record = json.loads(z.read(recordname))
            checksums = json.loads(z.read("0.0.43/checksums.json"))
        except (KeyError,ValueError,UnicodeError) as e:
            stop(f"ZIP nao e o pacote da compilacao: {type(e).__name__}")
    if (len(blob)!=EXPECTED_BIN_SIZE or sha(blob)!=EXPECTED_BIN_SHA
        or record.get("version")!=VERSION
        or record.get("sha256")!=EXPECTED_BIN_SHA
        or record.get("source_commit")!=EXPECTED_SOURCE
        or record.get("partitions_verified") is not True
        or record.get("credentials_compiled") is not False
        or checksums.get("firmware.ino.bin")!=EXPECTED_BIN_SHA
        or not blob.startswith(b"\xe9")
        or len(blob)>APP_SIZE):
        stop("Firmware HASH, origem ou particoes nao validados.")
    return blob

def ota_entries(raw: bytes):
    result=[]
    for sector in (0,1):
        offset=sector*4096
        seq=struct.unpack_from("<I",raw,offset)[0]
        state=struct.unpack_from("<I",raw,offset+24)[0]
        crc=struct.unpack_from("<I",raw,offset+28)[0]
        valid=(seq not in (0,0xffffffff)
               and crc==(binascii.crc32(struct.pack("<I",seq),0xffffffff)&0xffffffff)
               and state not in (3,4))
        result.append((seq,state,valid))
    return result

def patch_ota(raw: bytes, target_slot:int):
    if len(raw)!=8192 or target_slot not in (0,1):
        stop("otadata invalida.")
    entries=ota_entries(raw)
    valid=[(n,e[0]) for n,e in enumerate(entries) if e[2]]
    if valid:
        active,maxseq=max(valid,key=lambda row:row[1])
    else:
        active,maxseq=1,0
    if maxseq>=0xfffffff0:
        stop("Sequencia de otadata proxima do limite.")
    seq=maxseq+1
    if (seq-1)%2!=target_slot:
        seq+=1
    if seq==0 or seq>=0xffffffff:
        stop("Sequencia OTA invalida.")
    dest=1-active
    sector=bytearray(raw[dest*4096:(dest+1)*4096])
    # Only replace inactive 4KB sector; active one remains untouched if interrupted.
    struct.pack_into("<I",sector,0,seq)
    struct.pack_into("<I",sector,24,2)  # ESP_OTA_IMG_VALID
    struct.pack_into("<I",sector,28,binascii.crc32(struct.pack("<I",seq),0xffffffff)&0xffffffff)
    modified=bytearray(raw)
    modified[dest*4096:(dest+1)*4096]=sector
    assert ota_entries(modified)[dest][2]
    assert (seq-1)%2==target_slot
    return OTADATA+dest*4096,bytes(sector),seq

def do_switch(port, output_dir, target):
    # Always re-read otadata after the LAST operation in case app changed it.
    snapshot=output_dir/"otadata-before-switch.bin"
    esptool(port,"read-flash","0xe000","0x2000",str(snapshot))
    raw=snapshot.read_bytes()
    off, sector, seq=patch_ota(raw,target)
    proposed=output_dir/f"selector-slot-{target}.bin"
    proposed.write_bytes(sector)
    print(f"Switch OTA target app{target}, sector 0x{off:x}, seq={seq}")
    esptool(port,"write-flash",hex(off),str(proposed))
    # write-flash verifies its own hash and hard-resets the chip.
    return seq

def wait_for_lab(seconds=35):
    until=time.monotonic()+seconds
    while time.monotonic()<until:
        try:
            s=status(timeout=2)
            if (s.startswith("BOTIZIN WROOM V0.0.43\n")
                and "RUNNING_PARTITION: app1 / ota_1" in s
                and "VOICE_LAB_RETURN_BASE_ARMED: YES" in s
                and "VOICE_SYNC_EVENTS:" in s):
                return s
        except Exception:
            pass
        time.sleep(2)
    return None

def wait_for_base(seconds=25):
    until=time.monotonic()+seconds
    while time.monotonic()<until:
        try:
            s=status(timeout=2)
            if (s.startswith("BOTIZIN WROOM V0.0.40\n")
                and "RUNNING_PARTITION: app0 / ota_0" in s):
                return True
        except Exception:
            pass
        time.sleep(2)
    return False

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--package",required=True)
    ap.add_argument("--port",default="COM6")
    ap.add_argument("--backup-dir",required=True)
    ap.add_argument("--confirm",default="")
    args=ap.parse_args()
    if args.confirm!="PROMOTE_WROOM_APP1_043":
        stop("Precisa --confirm PROMOTE_WROOM_APP1_043")
    zip_path=pathlib.Path(args.package)
    if not zip_path.is_file():
        stop("ZIP nao encontrado.")
    if zip_path.suffix.lower()==".bin":
        blob=zip_path.read_bytes()
        if (len(blob)!=EXPECTED_BIN_SIZE or sha(blob)!=EXPECTED_BIN_SHA
            or not blob.startswith(b"\xe9")):
            stop("Binario nao confere tamanho, assinatura ou SHA256 esperado.")
    else:
        blob=package_from_zip(zip_path)
    print("LAB 0.0.43 validada:",len(blob),"bytes",sha(blob))
    check_old_status()
    check_serial(args.port)

    destination=pathlib.Path(args.backup_dir).resolve()
    destination.mkdir(parents=True,exist_ok=True)
    stamp=dt.datetime.now(dt.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    # The local backup contains WiFi/NVS secrets: NEVER upload/send it.
    backup=destination/f"wroom-full-4mb-before-face043-{stamp}.bin"
    esptool(args.port,"read-flash","0x0","0x400000",str(backup))
    original=backup.read_bytes()
    if len(original)!=FLASH_SIZE:
        stop("Falha no backup 4MB: nenhuma gravacao.")
    parse_partition_table(original)
    checksum=sha(original)
    backup.with_suffix(".sha256.txt").write_text(checksum+"  "+backup.name+"\n",encoding="ascii")
    print("BACKUP COMPLETO SALVO APENAS NO PC:",backup)
    print("BACKUP SHA256:",checksum)
    lab=destination/"wroom-face-stable-043.bin"
    lab.write_bytes(blob)
    print("Gravando SOMENTE OTA1 em 0x150000; OTA0/NVS intactos...")
    esptool(args.port,"write-flash","0x150000",str(lab))
    verify=destination/"wroom-app1-readback-043.bin"
    esptool(args.port,"read-flash","0x150000",str(len(blob)),str(verify))
    if sha(verify.read_bytes())!=EXPECTED_BIN_SHA:
        stop("Leitura OTA1 divergiu. Boot app1 NAO selecionado.")
    print("OTA1 gravada e verificada. Selecionando boot TEMPORARIO da LAB...")
    do_switch(args.port,destination,1)
    if wait_for_lab(35):
        print("LAB 0.0.43 ATIVA NA WROOM (OTA1).")
        print("CANDIDATA ESTA EM TESTE DE 60s. REINICIAR AGORA VOLTA A BASE .40.")
        print("AGUARDE CONFIRMACAO VOICE_STABLE_BOOT=CONFIRMED_APP1 ANTES DE DECLARAR PERSISTENTE.")
        return

    print("NAO FOI POSSIVEL CONFIRMAR A LAB. TENTANDO RETORNAR APP0...",flush=True)
    try:
        do_switch(args.port,destination,0)
        if wait_for_base(25):
            print("BASE 0.0.40 RESTAURADA AO BOOT.")
        else:
            print("BOOT BASE NAO CONFIRMADO; mantenha USB e backup local.")
    except Exception as e:
        print("FALHOU RETORNO AUTOMATICO:",type(e).__name__,str(e))
    stop("LAB nao validada, teste encerrado sem alterar APP0.")

if __name__=="__main__":
    main()
