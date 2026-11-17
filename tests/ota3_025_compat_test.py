"""Read-only build checks for the derived 0.0.25-OTA3 candidate."""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
fw=(root/'firmware/firmware.ino').read_text()
panel=(root/'firmware/panel_page.h').read_text()
route=(root/'firmware/control_routes.h').read_text()
version=(root/'firmware/version.h').read_text()
assert '"0.0.25-ota3"' in version
assert "OTA3_025_COMPAT_FROM_ORIGINAL" in fw
assert "OTA_AUTOMATIC_ENABLED: NO" in fw
assert "nextInternetCheck = 0;" in fw
assert "#if !BOTIZIN_OTA3_ENABLED" in fw
assert "sendTelemetry();" in fw and "pollPing();" in fw
assert "#include \"ota3_manager.h\"" in fw
assert "controlOTA3Switch" in route
assert "/control/ota3/catalog-refresh" in route
assert "/control/ota3/boot" in route
assert "controlRequireAuth()" in route
assert "controlSession" in route
assert "ota3-app0" in panel and "ota3-app1" in panel
assert "ota3-install-btn" in panel and "ota3-boot0" in panel
assert len(re.findall(r'R"BTZN\((.*?)\)BTZN"',panel,re.S))==1
print('OTA3_COMPAT_025_STATIC_OK')
