#!/usr/bin/env python3
"""No hardware: prove the PS installer retries after the esptool USB reset before any flash write."""
from pathlib import Path
s=Path("tools/wroom043/instalar-botizin-043.ps1").read_text(encoding="utf-8")
needle="ESPERANDO WROOM VOLTAR AO WIFI APOS RESET USB"
assert needle in s
assert "baseDeadline=[DateTime]::UtcNow.AddSeconds(110)" in s
assert "Start-Sleep -Seconds 3" in s
assert "BOOT_PARTITION: app0 / ota_0" in s
assert "RUNNING_PARTITION: app0 / ota_0" in s
assert "OTA: READY" in s
assert s.index("flash-id") < s.index(needle) < s.index("$baseStatus=$null")
assert s.index("if(!$baseStatus){throw") < s.index("Log '[5/7] BACKUP INTEGRAL + GRAVACAO APENAS APP1'")
assert s.index("Log '[5/7] BACKUP INTEGRAL + GRAVACAO APENAS APP1'") < s.index('& $py -u $installer')
print("WROOM043_POST_USB_RESET_WAIT_GUARD_OK")
