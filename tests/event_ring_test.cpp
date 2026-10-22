#include "../firmware/event_ring.h"
#include <cassert>
#include <cstdio>
#include <cstring>
int main() {
  static_assert(sizeof(EventRing)<4096,"4KB fixed ring limit");
  EventRing e;
  assert(e.count==0 && e.total==0 && e.overwritten()==0);
  assert(strcmp(eventName(EV_HTTP_REQUEST_OK),"HTTP_REQUEST_OK")==0);
  e.add(100,EV_BOOT,EV_INFO,"POWERON");
  assert(e.count==1 && e.oldest(0).ms==100 && strcmp(e.oldest(0).detail,"POWERON")==0);
  for(unsigned i=0;i<70;i++) {
    char detail[90];
    memset(detail,'x',sizeof(detail));
    detail[sizeof(detail)-1]=0;
    e.add(200+i,EV_OTA_CHECK,EV_INFO,detail);
  }
  assert(e.count==64 && e.total==71 && e.overwritten()==7);
  assert(e.oldest(0).ms==206 && e.oldest(63).ms==269);
  assert(strlen(e.oldest(63).detail)==sizeof(EventItem::detail)-1);
  assert(strcmp(eventName(e.oldest(63).code),"OTA_CHECK")==0);
  e.add(300,EV_HTTP_REQUEST_OK,EV_INFO,nullptr);
  assert(e.total==72 && e.overwritten()==8 && e.oldest(63).detail[0]==0);
  std::puts("EVENT_RING_HOST_TESTS_OK 64_fixed_slots_overflow_bounded_payload");
}
