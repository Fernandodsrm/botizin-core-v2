#!/data/data/com.termux/files/usr/bin/python
"""BOTIZIN Termux watchdog: starts sshd and maintains Linux PRoot supervisor."""
import fcntl, os, socket, subprocess, time
from pathlib import Path
HOME=Path.home()
PREFIX=Path(os.environ.get("PREFIX","/data/data/com.termux/files/usr"))
BASE=HOME/".local/share/botizin"
BASE.mkdir(parents=True,exist_ok=True)
def note(msg):
    with (BASE/"watchdog-host.log").open("a") as f:
        f.write(time.strftime("%Y-%m-%d %H:%M:%S ")+msg+"\n")
def port(n):
    try:
        with socket.create_connection(("127.0.0.1",n),timeout=1):return True
    except OSError:return False
def main():
    lock=(BASE/"watchdog.lock").open("w")
    try:fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
    except BlockingIOError:return
    note("Supervisor Termux ativo")
    child=None
    last_start=0
    while True:
        if not port(8022):
            try:
                p=subprocess.run([str(PREFIX/"bin/sshd")],capture_output=True,timeout=8)
                note("SSH: inicio solicitado; codigo "+str(p.returncode))
            except Exception as e:note("SSH: "+type(e).__name__)
        now=time.monotonic()
        if (child is None or child.poll() is not None) and now-last_start>30:
            try:
                with (BASE/"watchdog-proot.log").open("ab") as f:
                    child=subprocess.Popen([str(PREFIX/"bin/proot-distro"),"login",
                        "botizin-linux","--","python3","/root/botizin-mcp/botizin-supervisor.py"],
                        stdin=subprocess.DEVNULL,stdout=f,stderr=subprocess.STDOUT,
                        start_new_session=True)
                note("Linux: supervisor iniciado PID "+str(child.pid))
            except Exception as e:note("Linux: "+type(e).__name__)
            last_start=now
        time.sleep(15)
if __name__=="__main__":main()
