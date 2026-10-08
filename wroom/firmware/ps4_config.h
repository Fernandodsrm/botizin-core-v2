#pragma once
static uint8_t ps4LedButton=1; // DualShock cross (Bluepad32 BUTTON_A)
static bool savePS4Button(int button) {
  if(button!=1 && button!=2 && button!=4 && button!=8) return false;
  Preferences prefs; if(!prefs.begin("ps4-config",false)) return false;
  prefs.putUChar("led-button",button);
  bool ok=prefs.getUChar("led-button",0)==button; prefs.end();
  if(ok) ps4LedButton=button; return ok;
}
static void loadPS4Button() {
  Preferences prefs; if(prefs.begin("ps4-config",true)) {
    uint8_t b=prefs.getUChar("led-button",1); prefs.end();
    if(b==1 || b==2 || b==4 || b==8) ps4LedButton=b;
  }
}
