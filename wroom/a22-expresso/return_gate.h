#pragma once
#include <stdint.h>
// A soft restart must wait for the renderer to release the I2C bus.
// Repeated requests cannot extend the deadline; subtraction handles rollover.
class ReturnGate {
  bool requested=false;
  uint32_t deadline=0;
public:
  bool active() const{return requested;}
  void request(uint32_t now){if(!requested){requested=true;deadline=now+750;}}
  bool ready(uint32_t now,bool displayStopped)const{
    return requested&&displayStopped&&int32_t(now-deadline)>=0;
  }
};
