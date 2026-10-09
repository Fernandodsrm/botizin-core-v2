import base64,hashlib,json,os,pathlib,urllib.request
REPO='Fernandodsrm/botizin-core-v2'
def api(path,body=None,method=None):
    data=None if body is None else json.dumps(body).encode()
    req=urllib.request.Request('https://api.github.com/repos/'+REPO+path,data=data,method=method,headers={'Authorization':'Bearer '+os.environ['GH_TOKEN'],'Accept':'application/vnd.github+json','Content-Type':'application/json'})
    return json.load(urllib.request.urlopen(req,timeout=30))
def main():
    activation=json.loads(pathlib.Path('independent-release-storage.json').read_text())
    entries=[]
    for p in activation['packages']:
        candidates=[x for x in pathlib.Path(p['path']).rglob('manifest.json') if json.loads(x.read_text()).get('version')==p['version']]
        assert len(candidates)==1
        manifest=candidates[0];d=manifest.parent
        binary=next(x for x in [d/'firmware.ino.bin',d/'firmware.bin'] if x.is_file())
        content=binary.read_bytes();assert len(content)==p['size'] and hashlib.sha256(content).hexdigest()==p['sha256']
        m=json.loads(manifest.read_text());assert m['size']==p['size'] and m['sha256']==p['sha256']
        record=json.loads((d/'build-record.json').read_text());assert record['source_commit']==activation['source_commit'] and record['partitions_verified']
        blob=api('/git/blobs',{'content':base64.b64encode(content).decode(),'encoding':'base64'})
        prefix='wroom/releases/'+p['version']+'/'
        entries.append({'path':prefix+'firmware.bin','mode':'100644','type':'blob','sha':blob['sha']})
        for name in ['manifest.json','build-record.json']:
            entries.append({'path':prefix+name,'mode':'100644','type':'blob','content':(d/name).read_text()})
    before=api('/git/ref/heads/main')['object']['sha']
    base=api('/git/commits/'+before)['tree']['sha']
    tree=api('/git/trees',{'base_tree':base,'tree':entries})
    commit=api('/git/commits',{'message':'Store verified standalone module packages without activation','tree':tree['sha'],'parents':[before]})
    assert api('/git/ref/heads/main')['object']['sha']==before,'main moved; no ref update'
    api('/git/refs/heads/main',{'sha':commit['sha'],'force':False},'PATCH')
    print('RELEASE_FILES_STORED; MANIFEST_NOT_CHANGED',commit['sha'])
if __name__=='__main__':main()
