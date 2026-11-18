"""OTA3 0.0.31: static UX contract for clear, manual, safe visual feedback."""
from pathlib import Path
from html.parser import HTMLParser
import re
root=Path(__file__).resolve().parents[1]
panel=(root/"firmware/panel_page.h").read_text()
fw=(root/"firmware/firmware.ino").read_text()
m=(root/"firmware/ota3_manager.h").read_text()
html=re.search(r'R"BTZN\((.*?)\)BTZN"',panel,re.S).group(1)

class Controls(HTMLParser):
    def __init__(self):
        super().__init__();self.ids=[];self.buttons={}
    def handle_starttag(self,tag,attrs):
        props=dict(attrs)
        if "id" in props:
            self.ids.append(props["id"])
            if tag=="button":self.buttons[props["id"]]=props
    handle_startendtag=handle_starttag

controls=Controls();controls.feed(html)
assert len(controls.ids)==len(set(controls.ids)),"IDs duplicados no HTML"
required=[
    "ota3-card0","ota3-card1","ota3-tag0","ota3-tag1",
    "ota3-app0","ota3-app1","ota3-detail0","ota3-detail1",
    "ota3-status-btn","ota3-catalog-btn","ota3-destination",
    "ota3-install-btn","ota3-version","ota3-result",
    "ota3-output","ota3-boot0","ota3-boot1","ota3-feedback"
]
for item in required:assert item in controls.ids,item
for item in ["ota3-status-btn","ota3-catalog-btn","ota3-install-btn",
             "ota3-boot0","ota3-boot1"]:
    assert item in controls.buttons
for frag in ["Atualizar informações das APPs","Instalar na APP ","ota3Present(data)",
             "ota3UpdateDestination()","APP0_RUNNING","APP1_RUNNING",
             "if(data.APP0_RUNNING||data.APP1_RUNNING)ota3Present(data)",
             "await ota3GetStatus()","ota3Installing=false",
             "Iniciar APP 0","Iniciar APP 1""]:
    assert frag in panel,frag
assert "Ler partições" not in panel
assert "APP "+ "inativa" not in controls.buttons.get("ota3-install-btn",{}).get("id","")
assert 'CANDIDATE_BUILD: OTA3_CANDIDATE_031' in fw
assert 'OTA_AUTOMATIC_ENABLED: NO' in fw and 'CHECK_INTERVAL_SECONDS: 0' in fw
assert 'esp_ota_begin(dest,entry.size' in m and 'esp_ota_set_boot_partition(chosen)' in m
assert 'bootAfter->address==bootBefore->address' in m
assert 'ota3StoreSlotMetadata' in m and 'ota3ResolveVersion' in m
assert "0.0.31" in (root/"firmware/version.h").read_text()
print("OTA3_031_PANEL_CONTRACT_OK")
