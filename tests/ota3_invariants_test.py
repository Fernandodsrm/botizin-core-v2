"""OTA3 candidate contract tests: static gates before compilation."""
from pathlib import Path
import json,re
root=Path(__file__).resolve().parents[1]
s=(root/"firmware/firmware.ino").read_text()
p=(root/"firmware/panel_page.h").read_text()
r=(root/"firmware/control_routes.h").read_text()
m=(root/"firmware/ota3_manager.h").read_text()
policy=(root/"firmware/lab_policy.h").read_text()
catalog=json.loads((root/"ota3/catalog.json").read_text())
assert '#define BOTIZIN_OTA3_ENABLED 1' in policy
assert 'OTA_AUTOMATIC_ENABLED: NO' in s and 'CHECK_INTERVAL_SECONDS: 0' in s
assert 'nextInternetCheck = 0;' in s
assert '#if !BOTIZIN_LAB_OTA_DISABLED && !BOTIZIN_OTA3_ENABLED' in s
assert 'esp_ota_begin(dest,entry.size' in m and 'esp_ota_write(handle,buffer,got)' in m
assert 'esp_ota_end(handle)' in m and 'hashPartition(dest,entry.size,flashSha)' in m
assert 'bootAfter->address==bootBefore->address' in m
assert 'esp_ota_set_boot_partition(chosen)' in m
assert 'dest==esp_ota_get_running_partition()' in m
for frag in ['/control/ota3/catalog-refresh','/control/ota3/catalog',
             '/control/ota3/install','/control/ota3/boot']:
    assert frag in r and frag in p,frag
assert 'controlRequireAuth()' in r
assert len(catalog['entries'])>=2
assert catalog['board']=='esp32s3-n16r8'
ids=set()
for e in catalog['entries']:
    assert e['id'] not in ids
    ids.add(e['id'])
    assert e['approved'] is True
    assert 4096<=e['size']<=3145728
    assert re.fullmatch(r'[a-f0-9]{64}',e['sha256'])
    assert e['url'].startswith('https://raw.githubusercontent.com/Fernandodsrm/botizin-core-v2/')
    assert e['board']==catalog['board']
assert all('secrets.h' not in x for x in (s,m,p))
assert 'CATALOGO_INVALIDO' in m and 'FLASH_OU_BOOT_NAO_CONFEREM' in m
assert len(re.findall(r'R"BTZN\((.*?)\)BTZN"',p,re.S))==1
print('OTA3_CONTRACT_STATIC_OK')
