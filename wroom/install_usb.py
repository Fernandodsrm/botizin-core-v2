"""Install WROOM 0.0.1 after validating its package and full recovery backup."""
import argparse,hashlib,json,pathlib,subprocess
from serial.tools import list_ports
p=argparse.ArgumentParser();p.add_argument('--package',required=True);p.add_argument('--backup',required=True);p.add_argument('--esptool',required=True);p.add_argument('--port',default='COM6');a=p.parse_args()
backup=pathlib.Path(a.backup)
if backup.stat().st_size!=4194304 or hashlib.sha256(backup.read_bytes()).hexdigest()!='670ebe0e155e1e0cfe8169e55397e9e792e40bb23d846773d941a96643289e37':raise SystemExit('STOP: recovery backup does not match the verified WROOM backup')
root=pathlib.Path(a.package);record=json.loads((root/'build-record.json').read_text());checks=json.loads((root/'checksums.json').read_text())
if record['version']!='0.0.1' or not record['partitions_verified'] or record['credentials_compiled']:raise SystemExit('STOP: wrong package')
segments=[('0x1000','firmware.ino.bootloader.bin',0x7000),('0x8000','firmware.ino.partitions.bin',0x1000),('0xe000','boot_app0.bin',0x2000),('0x10000','firmware.ino.bin',0x140000)]
for offset,name,limit in segments:
    data=(root/name).read_bytes()
    if not data or len(data)>limit or hashlib.sha256(data).hexdigest()!=checks.get(name):raise SystemExit('STOP: package checksum/size failed '+name)
if hashlib.sha256((root/'firmware.ino.bin').read_bytes()).hexdigest()!=record['sha256']:raise SystemExit('STOP: build record mismatch')
ports=[x for x in list_ports.comports() if x.device.upper()==a.port.upper()]
if len(ports)!=1 or (ports[0].vid,ports[0].pid)!=(0x1a86,0x55d4):raise SystemExit('STOP: expected CH9102 WROOM adapter')
base=[a.esptool,'--chip','esp32','--port',a.port]
check=subprocess.run(base+['flash-id'],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);print(check.stdout)
if check.returncode or '30:76:f5:90:67:e0' not in check.stdout.lower() or 'Detected flash size: 4MB' not in check.stdout:raise SystemExit('STOP: chip identity or flash size does not match')
command=base+['--baud','115200','write-flash','--flash-mode','dio','--flash-size','4MB']
for offset,name,limit in segments:command += [offset,str(root/name)]
print('Installing BOTIZIN WROOM 0.0.1. Bootloader, partition table, OTA selection and app0 will be written; NVS and SPIFFS ranges are preserved.',flush=True)
subprocess.run(command,check=True)
print('USB WRITE COMPLETED. Boot, Wi-Fi, telemetry and OTA still require physical verification.')
