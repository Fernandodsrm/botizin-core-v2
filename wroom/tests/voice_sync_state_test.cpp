#include <cassert>
#include <cstdint>
#include <cstdio>
#include "../firmware/voice_sync_state.h"
int main(){
  BotizinVoiceSync s;
  assert(s.mode(100)==0);
  assert(!s.receive(1,100));
  assert(!s.receive(9,100));
  assert(s.accepted.load()==0);
  assert(s.receive(5,100));
  assert(s.mode(100)==5);
  assert(s.mode(4100)==5);
  assert(s.mode(4101)==0);
  assert(s.receive(6,5000));
  assert(s.mode(5500)==6);
  assert(s.receive(3,6000));
  assert(s.mode(6010)==3);
  assert(s.receive(4,0xfffffff0u));
  assert(s.mode(0x00000020u)==4);
  assert(s.mode(0x00001000u)==0);
  assert(s.accepted.load()==4);
  puts("PASS: mode input, idle/listening/speaking, TTL, timer rollover");
}
