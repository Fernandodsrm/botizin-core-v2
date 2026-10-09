"""Compile Bluetooth-only module against pinned previously used Bluepad32 core."""
import hashlib,json,os,pathlib,shutil,struct,subprocess
ROOT=pathlib.Path(__file__).resolve().parents[1]
FQBN='esp32-bluepad32:esp32:esp32:FlashSize=4M,FlashMode=dio,CPUFreq=240,EraseFlash=none'
version='0.0.23'
def main():
 source=ROOT/'wroom/ps4-module';out=ROOT/'ps4-output';out.mkdir(exist_ok=True)
 target=ROOT/'dist-ps4'/version;target.mkdir(parents=True,exist_ok=True)
 with (target/'compile.log').open('w') as log:
  p=subprocess.Popen(['arduino-cli','compile','--fqbn',FQBN,'--output-dir',str(out),str(source)],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
  for line in p.stdout:print(line,end='',flush=True);log.write(line)
  assert p.wait()==0,'PS4 compilation failed; no publication'
 binary=(out/'ps4-module.ino.bin').read_bytes();assert binary[0]==0xe9 and struct.unpack_from('<H',binary,12)[0]==0
 assert len(binary)<=0x140000,'PS4 module does not fit preserved OTA slots'
 table=(out/'ps4-module.ino.partitions.bin').read_bytes()
 expected=[('nvs',1,2,0x9000,0x5000),('otadata',1,0,0xe000,0x2000),('app0',0,16,0x10000,0x140000),('app1',0,17,0x150000,0x140000),('spiffs',1,130,0x290000,0x160000),('coredump',1,3,0x3f0000,0x10000)]
 for i,e in enumerate(expected):
  magic,typ,sub,off,size,label,flags=struct.unpack_from('<HBBII16sI',table,i*32)
  assert magic==0x50aa and (label.rstrip(b'\0').decode(),typ,sub,off,size)==e and flags==0
 sha=hashlib.sha256(binary).hexdigest();(target/'firmware.bin').write_bytes(binary)
 shutil.copy2(out/'ps4-module.ino.elf',target/'firmware.elf')
 manifest={'board':'esp32-wroom-4mb','version':version,'size':len(binary),'sha256':sha,'url':'https://raw.githubusercontent.com/Fernandodsrm/botizin-core-v2/main/wroom/releases/'+version+'/firmware.bin'}
 (target/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
 record={**manifest,'source_commit':os.environ.get('GITHUB_SHA'),'core':'esp32-bluepad32:esp32@4.1.0','credentials_compiled':False,'partitions_verified':True,'return_requires':'Menu 0.0.22 plus verified NVS return anchor','wifi_started':False}
 (target/'build-record.json').write_text(json.dumps(record,indent=2)+'\n');print(json.dumps(record),flush=True)
if __name__=='__main__':main()
