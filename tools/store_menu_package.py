import os,json,base64,hashlib,urllib.request,urllib.error
from pathlib import Path
p=Path('menu-package');m=json.loads((p/'manifest.json').read_text());r=json.loads((p/'build-record.json').read_text());b=(p/'firmware.bin').read_bytes()
assert r['source_commit']=='c2eda9ccfc83644fc07b774f4a78fd71259e6cc6' and r['baseline_version']=='0.0.15' and r['baseline_sha256']=='5428313389336d48831e9228fb98d18603f848d44fdd1557a3de337f1b45580c'
assert r['partitions_verified'] and not r['credentials_compiled'] and r['return_seconds']==600
assert m['version']=='0.0.17' and m['board']=='esp32-wroom-4mb' and len(b)==m['size']==1162544
assert m['sha256']==hashlib.sha256(b).hexdigest()=='75aac11d82cf1daabcdbdd1a94922f837c91bae8105b0e9d94462af9d412b733'
assert b'Voltando...' in b and b'ARMED_600_SECONDS_RETURN_0.0.15' in b
base='https://api.github.com/repos/Fernandodsrm/botizin-core-v2/contents/'
def req(path,body=None):
 q=urllib.request.Request(base+path+('?ref=main' if body is None else ''),data=json.dumps(body).encode() if body else None,headers={'Authorization':'Bearer '+os.environ['GITHUB_TOKEN'],'Accept':'application/vnd.github+json'},method='PUT' if body else 'GET')
 return json.load(urllib.request.urlopen(q,timeout=45))
for name,data in [('firmware.bin',b),('manifest.json',(p/'manifest.json').read_bytes()),('build-record.json',(p/'build-record.json').read_bytes())]:
 path='wroom/releases/0.0.17/'+name
 try:old=req(path)
 except urllib.error.HTTPError as e:
  if e.code!=404:raise
  old=None
 if old:
  assert old['size']==len(data) and old['sha']==hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest()
 else:req(path,{'branch':'main','message':'Store verified Menu 0.0.17 package; no automatic offer','content':base64.b64encode(data).decode()})
print('MENU_PACKAGE_STORED_NO_OFFER',len(b),m['sha256'])
