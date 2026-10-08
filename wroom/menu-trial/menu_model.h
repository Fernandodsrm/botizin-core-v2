#pragma once
#include <stdint.h>
struct MenuModel {
  uint8_t selected = 0;
  bool detail = false;
  bool confirmingReturn = false;
  bool ledOn = false;
  bool handle(uint8_t event) {
    // Priority is BACK, then navigation, then OK. Stale navigation cannot confirm.
    if (event & 4) { detail = false; confirmingReturn = false; ledOn = false; return false; }
    if (!detail) {
      if (event & (2 | 8)) selected = (selected + ((event & 8) ? 1 : 4)) % 5;
      else if (event & 1) { detail = true; confirmingReturn = selected == 4; }
    } else if ((event & 1) && !(event & (2 | 8))) {
      if (confirmingReturn) { ledOn = false; return true; }
      if (selected == 0) ledOn = !ledOn;
    }
    return false;
  }
};
