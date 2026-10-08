"""Publish only a pinned, verified package; leave the OTA implementation unchanged."""
import os,json,base64,hashlib,urllib.request,urllib.error
from pathlib import Path
REPO='Fernandodsrm/botizin-core-v2'
def request(path,body=None):
 req=urllib.request.Request('https://api.github.com/repos/'+REPO+'/contents/'+path+('?ref=main' if body is None else ''),data=json.dumps(body).encode() if body else None,headers={'Authorization':'Bearer '+os.environ['GITHUB_TOKEN'],'Accept':'application/vnd.github+json'},method='PUT' if body else 'GET')
 return json.load(urllib.request.urlopen(req,timeout=45))
def put(path,data,message,sha=None):
 b={'branch':'main','message':message,'content':base64.b64encode(data).decode()}
 if sha:b['sha']=sha
 return request(path,b)
def decode(r):return base64.b64decode(r['content'])
def main():
 e=os.environ;w=e['DEVICE']=='WROOM';folder=Path('release-package')/('0.0.15' if w else '')
 m=json.loads((folder/'manifest.json').read_text());r=json.loads((folder/'build-record.json').read_text())
 data=(folder/('firmware.ino.bin' if w else 'firmware.bin')).read_bytes()
 assert r['source_commit']==e['SOURCE_COMMIT'] and m['version']==r['version']==e['VERSION']
 assert m['board']==('esp32-wroom-4mb' if w else 'esp32s3-n16r8')
 assert len(data)==m['size']==r['bytes']==int(e['IMAGE_BYTES']) and data[0]==0xe9
 assert m['sha256']==r['sha256']==hashlib.sha256(data).hexdigest()==e['IMAGE_SHA256']
 assert len(data)<=(1310720 if w else 3145728)
 if w:assert r['partitions_verified'] and not r['credentials_compiled']
 else:assert r['publishable'] and r['device_token_compiled'] and not r['wifi_credentials_compiled']
 prefix='wroom/' if w else '';manifest_path=prefix+'manifest.json';path=prefix+'releases/'+m['version']+'/firmware.bin'
 assert m['url']=='https://raw.githubusercontent.com/'+REPO+'/main/'+path
 old=request(manifest_path);old_data=decode(old);om=json.loads(old_data)
 assert om['version']==e['BASE_VERSION'] and om['sha256']==e['BASE_SHA256']
 try:stored=request(path)
 except urllib.error.HTTPError as exc:
  if exc.code!=404:raise
  stored=None
 if stored:
  assert stored['size']==len(data) and stored['sha']==hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest()
 else:put(path,data,'Store verified '+e['DEVICE']+' '+m['version']+' local-name discovery firmware')
 put(manifest_path,(json.dumps(m,indent=2)+'\n').encode(),'Offer verified '+e['DEVICE']+' '+m['version']+' address discovery correction',old['sha'])
 assert json.loads(decode(request(manifest_path)))==m
 print('RELEASE_OFFERED',e['DEVICE'],m['version'],len(data),m['sha256'],flush=True)
if __name__=='__main__':main()
