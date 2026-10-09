"""Build distinct classic-ESP32 images, with no publication and no secrets."""
import hashlib, json, os, pathlib, shutil, struct, subprocess
ROOT = pathlib.Path(__file__).resolve().parents[1]
FQBN = "esp32:esp32:esp32:FlashSize=4M,FlashMode=dio,CPUFreq=240,EraseFlash=none"
EXPECTED = [("nvs",1,2,0x9000,0x5000),("otadata",1,0,0xe000,0x2000),
            ("app0",0,16,0x10000,0x140000),("app1",0,17,0x150000,0x140000),
            ("spiffs",1,130,0x290000,0x160000),("coredump",1,3,0x3f0000,0x10000)]
def main():
    dist = ROOT / "dist-wroom"; dist.mkdir(exist_ok=True)
    for helper in ("install_usb.py","provision_usb.py","README.md"):
        shutil.copy2(ROOT/"wroom"/helper,dist/helper)
    original = ROOT / "wroom/firmware"
    cores = json.loads(subprocess.check_output(["arduino-cli","core","list","--format","json"],text=True))
    assert any(p.get("id")=="esp32:esp32" and p.get("installed_version")=="3.3.12" for p in cores["platforms"])
    subprocess.run(["arduino-cli","board","details","--fqbn",FQBN],check=True)
    for version in ("0.0.22",):
        source = ROOT / "wroom-build" / "firmware"; source.parent.mkdir(exist_ok=True)
        shutil.copytree(original,source,dirs_exist_ok=True)
        (source/"version.h").write_text('#pragma once\n#define BOTIZIN_VERSION "'+version+'"\n')
        out = ROOT / "wroom-output" / version; out.mkdir(parents=True,exist_ok=True)
        target = dist/version; target.mkdir(exist_ok=True)
        with (target/"compile.log").open("w") as log:
            process=subprocess.Popen(["arduino-cli","compile","--fqbn",FQBN,"--output-dir",str(out),str(source)],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
            for line in process.stdout: print(line,end="",flush=True);log.write(line)
            assert process.wait()==0,"Compilation failed; nothing published"
        binary=(out/"firmware.ino.bin").read_bytes()
        assert binary[0]==0xe9 and struct.unpack_from("<H",binary,12)[0]==0,"Wrong chip"
        assert len(binary)<=0x140000,"Does not fit preserved OTA partition"
        assert b"BOTIZIN WROOM V" in binary and version.encode() in binary
        table=(out/"firmware.ino.partitions.bin").read_bytes()
        for i,expected in enumerate(EXPECTED):
            magic,typ,sub,offset,size,label,flags=struct.unpack_from("<HBBII16sI",table,i*32)
            assert magic==0x50aa and (label.rstrip(b"\x00").decode(),typ,sub,offset,size)==expected and flags==0,"Partition mismatch"
        for filename in ("firmware.ino.bin","firmware.ino.bootloader.bin","firmware.ino.partitions.bin","firmware.ino.elf"):
            shutil.copy2(out/filename,target/filename)
        boot_app0=pathlib.Path.home()/".arduino15/packages/esp32/hardware/esp32/3.3.12/tools/partitions/boot_app0.bin"
        assert boot_app0.stat().st_size == 8192
        shutil.copy2(boot_app0,target/"boot_app0.bin")
        sha=hashlib.sha256(binary).hexdigest()
        manifest={"board":"esp32-wroom-4mb","version":version,"size":len(binary),"sha256":sha,"url":"https://raw.githubusercontent.com/Fernandodsrm/botizin-core-v2/main/wroom/releases/"+version+"/firmware.bin"}
        (target/"manifest.json").write_text(json.dumps(manifest,indent=2)+"\n")
        record={"version":version,"bytes":len(binary),"sha256":sha,"fqbn":FQBN,"source_commit":os.environ.get("GITHUB_SHA"),"published":False,"credentials_compiled":False,"partitions_verified":True}
        (target/"build-record.json").write_text(json.dumps(record,indent=2)+"\n")
        (target/"checksums.json").write_text(json.dumps({p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in target.glob("*.bin")},indent=2)+"\n")
        print(json.dumps(record),flush=True)
if __name__=="__main__": main()
