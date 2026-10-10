"""Publish one verified A22 environment, preserve catalog entries and live Menu."""
import base64,hashlib,json,os,pathlib,urllib.request
REPO='Fernandodsrm/botizin-core-v2'
def api(path,body=None,method=None):
    data=None if body is None else json.dumps(body).encode()
    req=urllib.request.Request('https://api.github.com/repos/'+REPO+path,data=data,method=method,headers={'Authorization':'Bearer '+os.environ['GH_TOKEN'],'Accept':'application/vnd.github+json','Content-Type':'application/json'})
    return json.load(urllib.request.urlopen(req,timeout=30))
def read_json(path):
    value=api('/contents/'+path+'?ref=main')
    return json.loads(base64.b64decode(value['content']))
def main():
    config=json.loads(pathlib.Path('a22-release-storage.json').read_text())
    candidates=[p for p in pathlib.Path('artifact-a22').rglob('manifest.json') if json.loads(p.read_text()).get('version')==config['version']]
    assert len(candidates)==1
    d=candidates[0].parent;m=json.loads(candidates[0].read_text())
    binary=(d/'a22-expresso.ino.bin').read_bytes();sha=hashlib.sha256(binary).hexdigest()
    assert len(binary)==config['size']==m['size'] and sha==config['sha256']==m['sha256']
    record=json.loads((d/'build-record.json').read_text())
    assert record['source_commit']==config['source_commit'] and record['partitions_verified'] and not record['credentials_compiled']
    assert len(binary)<=0x140000 and m['board']=='esp32-wroom-4mb'
    assert m['url']=='https://raw.githubusercontent.com/'+REPO+'/main/wroom/releases/'+config['version']+'/firmware.bin'
    before=api('/git/ref/heads/main')['object']['sha'];base=api('/git/commits/'+before)['tree']['sha']
    catalog=read_json('wroom/catalog.json');menu=read_json('wroom/manifest.json')
    assert catalog['schema']==1 and menu['version']=='0.0.29'
    previous={e['id']:e for e in catalog['environments']}
    assert {'ps4','servo','led'}.issubset(previous)
    entry={**m,'id':'a22-expresso','title':'A22 Expresso','return_protocol':2}
    entries=[e for e in catalog['environments'] if e['id']!='a22-expresso']+[entry]
    assert len(entries)<=8
    catalog['environments']=entries
    blob=api('/git/blobs',{'content':base64.b64encode(binary).decode(),'encoding':'base64'})
    prefix='wroom/releases/'+config['version']+'/'
    files=[{'path':prefix+'firmware.bin','mode':'100644','type':'blob','sha':blob['sha']},
           {'path':'wroom/catalog.json','mode':'100644','type':'blob','content':json.dumps(catalog,indent=2)+'\n'}]
    for name in ['manifest.json','build-record.json']:
        files.append({'path':prefix+name,'mode':'100644','type':'blob','content':(d/name).read_text()})
    tree=api('/git/trees',{'base_tree':base,'tree':files})
    commit=api('/git/commits',{'message':'Publish verified A22 Expresso environment and extend catalog; preserve Menu 0.0.29','tree':tree['sha'],'parents':[before]})
    assert api('/git/ref/heads/main')['object']['sha']==before,'main moved; publication cancelled'
    api('/git/refs/heads/main',{'sha':commit['sha'],'force':False},'PATCH')
    print('A22_PUBLISHED_MENU_UNCHANGED',commit['sha'])
if __name__=='__main__':main()
