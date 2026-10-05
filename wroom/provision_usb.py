"""Configure only the WROOM device over USB; passwords are not logged."""
import argparse,getpass,json,time
import serial
from serial.tools import list_ports
p=argparse.ArgumentParser();p.add_argument('--port',default='COM6');a=p.parse_args()
ports=[x for x in list_ports.comports() if x.device.upper()==a.port.upper()]
if len(ports)!=1 or (ports[0].vid,ports[0].pid)!=(0x1a86,0x55d4):raise SystemExit('STOP: expected CH9102 WROOM USB adapter')
ssid=input('Nome do Wi-Fi (2.4 GHz): ').strip()
password=getpass.getpass('Senha do Wi-Fi (nao aparece): ')
token=getpass.getpass('Token de acesso do BOTIZIN-WROOM (nao aparece): ').strip()
if not 1<=len(ssid.encode())<=32 or not (len(password)==0 or 8<=len(password.encode())<=63):raise SystemExit('Wi-Fi configuration invalid')
if not 16<=len(token)<=128 or token.startswith('tb_') or not all(c.isascii() and (c.isalnum() or c in '_-') for c in token):raise SystemExit('Device token invalid; do not use account API key')
line=json.dumps({'cmd':'configure','ssid':ssid,'password':password,'device_token':token},ensure_ascii=False,separators=(',',':')).encode()+b'\n'
if len(line)>511:raise SystemExit('Configuration too large')
s=serial.Serial();s.port=a.port;s.baudrate=115200;s.timeout=.5;s.dtr=False;s.rts=False;s.open()
print('Pressione RESET/EN uma vez. Aguardando a identidade BOTIZIN WROOM...')
end=time.monotonic()+50;confirmed=False
while time.monotonic()<end:
    text=s.readline().decode('utf-8','replace').strip()
    if text.startswith('BOTIZIN WROOM V'):confirmed=True;break
if not confirmed:s.close();raise SystemExit('STOP: WROOM firmware identity not observed; nothing configured')
# Setup takes up to 30 seconds if existing Wi-Fi credentials are unreachable.
time.sleep(1);s.write(line);s.flush();line=b'';password=token=''
end=time.monotonic()+50;saved=False
while time.monotonic()<end:
    text=s.readline().decode('utf-8','replace').strip()
    if text.startswith('CONFIG_SAVED_REBOOT'):saved=True;print('CONFIGURACAO SALVA. Placa reiniciando.');break
    if text.startswith(('CONFIG_REJECTED','CONFIG_SAVE_FAILED')):s.close();raise SystemExit(text)
s.close()
if not saved:raise SystemExit('No save confirmation; inspect serial before retrying')
print('Abra o monitor serial para conferir IP, WIFI: OK e TELEMETRY: HTTP_200.')
