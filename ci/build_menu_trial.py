"""Compile temporary WROOM Menu; never publish or upload to a board."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess

ROOT=Path(__file__).resolve().parents[1]
FQBN='esp32:esp32:esp32:FlashSize=4M,FlashMode=dio,CPUFreq=240,EraseFlash=none'
EXPECTED=[('nvs',1,2,0x9000,0x5000),('otadata',1,0,0xe000,0x2000),
          ('app0',0,16,0x10000,0x140000),('app1',0,17,0x150000,0x140000),
          ('spiffs',1,130,0x290000,0x160000),('coredump',1,3,0x3f0000,0x10000)]

def main():
    cores=json.loads(subprocess.check_output(['arduino-cli','core','list','--format','json'],text=True))
    assert any(p.get('id')=='esp32:esp32' and p.get('installed_version')=='3.3.12' for p in cores['platforms'])
    dist=ROOT/'dist-menu-trial';dist.mkdir(exist_ok=True)
    out=ROOT/'menu-trial-build';out.mkdir(exist_ok=True)
    command=['arduino-cli','compile','--fqbn',FQBN,'--output-dir',str(out),str(ROOT/'wroom/menu-trial')]
    with (dist/'compile.log').open('w') as log:
        process=subprocess.Popen(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
        for line in process.stdout: print(line,end='',flush=True);log.write(line)
        assert process.wait()==0,'Build failed; not offered'
    image=(out/'menu-trial.ino.bin').read_bytes()
    assert image[0]==0xe9 and struct.unpack_from('<H',image,12)[0]==0
    assert len(image)<=0x140000
    assert b'BOTIZIN_MENU_TRIAL_0.0.14_WRITES_BLOCKED' in image
    assert b'ARMED_600_SECONDS_RETURN_0.0.12' in image
    table=(out/'menu-trial.ino.partitions.bin').read_bytes()
    for i,expected in enumerate(EXPECTED):
        magic,typ,sub,offset,size,label,flags=struct.unpack_from('<HBBII16sI',table,i*32)
        assert magic==0x50aa and (label.rstrip(b'\0').decode(),typ,sub,offset,size)==expected and flags==0
    shutil.copyfile(out/'menu-trial.ino.bin',dist/'firmware.bin')
    sha=hashlib.sha256(image).hexdigest()
    manifest={'board':'esp32-wroom-4mb','version':'0.0.14','size':len(image),'sha256':sha,
              'url':'https://raw.githubusercontent.com/Fernandodsrm/botizin-core-v2/main/wroom/releases/0.0.14/firmware.bin'}
    record={'version':'0.0.14','module_id':'menu_led_trial','bytes':len(image),'sha256':sha,
            'source_commit':os.environ.get('GITHUB_SHA'),'fqbn':FQBN,'partitions_verified':True,
            'credentials_compiled':False,'published':False,'return_seconds':600,'baseline_version':'0.0.12',
            'baseline_sha256':'11a0a1c68a8b7bd7c9c1bd7782df070ece556e2ff1eec94f3899e01e4c6aa2f5'}
    (dist/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    (dist/'build-record.json').write_text(json.dumps(record,indent=2)+'\n')
    print(json.dumps(record))

if __name__=='__main__':main()
