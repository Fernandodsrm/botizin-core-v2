#include <cstdint>
#include <cstdio>
#include <cassert>
#define ARDUINO_ISR_ATTR
#define INPUT_PULLUP 0
#define FALLING 0
#define portMUX_INITIALIZER_UNLOCKED 0
using portMUX_TYPE=int;
#define portENTER_CRITICAL_ISR(x) ((void)x)
#define portEXIT_CRITICAL_ISR(x) ((void)x)
#define portENTER_CRITICAL(x) ((void)x)
#define portEXIT_CRITICAL(x) ((void)x)
uint32_t testNow=1000;uint32_t millis(){return testNow;}
void pinMode(uint8_t,int){}
void attachInterruptArg(uint8_t,void(*)(void*),void*,int){}
struct SerialMock{void println(const char*){} template<typename... T>void printf(const char*,T...){} }Serial;
bool uploadActive=false,uploadOK=false,rebootScheduled=false,otaCheckQueued=false,otaInstallQueued=false;
bool oledMenu=true,oledDetail=false;
uint8_t oledMenuChoice=0,oledOtaChoice=0,oledPage=0;
uint32_t nextOledRefresh=0;
const char *otaCandidateId="id",*otaTargetVersion="0.0.23",*otaTargetSHA="sha",*telemetryBootId="boot";
bool moduleAvailable=false;const char *moduleSHA="module";int moduleStarts=0;
bool queueModuleStart(const char*,const char*){++moduleStarts;return true;}
bool candidate=false,window=false;int installs=0,checks=0,cancels=0;
bool candidateReady(){return candidate;}
bool manualWindowActive(){return window;}
bool queueOtaCheck(){++checks;return true;}
bool confirmOta(const char*,const char*,const char*,const char*){++installs;return true;}
void cancelOtaCheck(){++cancels;candidate=false;}
#include "../firmware/face_state.h"
bool moduleStartQueued=false;
#include "../firmware/navigation_model.h"
#include "../firmware/navigation.h"
void press(uint8_t p){navPending=p;pollNavigation();}
int main(){
 beginNavigation();discardNavigation();
 // The first press dismisses the character, never selects or installs.
 assert(faceActive.load());press(1);assert(!faceActive.load()&&navLevel==0&&checks==0);
 testNow=30999;pollFaceIdle();assert(!faceActive.load());
 testNow=31000;pollFaceIdle();assert(faceActive.load());
 press(4);assert(!faceActive.load()&&navLevel==0);
 testNow=62000;uploadActive=true;pollFaceIdle();assert(!faceActive.load());uploadActive=false;
 testNow=91999;pollFaceIdle();assert(!faceActive.load());testNow=92000;pollFaceIdle();assert(faceActive.load());
 press(8);assert(navLevel==0&&oledMenuChoice==0&&!faceActive.load());
 candidate=true;testNow+=30000;pollFaceIdle();assert(!faceActive.load());candidate=false;
 oledDetail=true;testNow+=30000;pollFaceIdle();assert(!faceActive.load());oledDetail=false;
 assert(!faceIdleDue(20000,0xfffffff0u,false));assert(faceIdleDue(30000,0xfffffff0u,false));

 // Local connection opens directly, with no board selector.
 press(1);assert(navLevel==3&&oledPage==8);press(8);assert(oledPage==8);press(4);assert(navLevel==0);
 // Diagnostics category back returns directly to root.
 press(8);press(1);assert(navLevel==2);press(8);press(1);assert(navLevel==3&&oledPage==7&&navDiagnostic==1);
 press(4);assert(navLevel==2);press(4);assert(navLevel==0);
 // Query, expired candidate, explicit second confirmation and cancellation.
 oledMenuChoice=2;press(1);assert(oledPage==2&&navLevel==3);
 press(1);assert(checks==1);press(8);press(1);assert(installs==0&&!oledDetail);
 candidate=true;window=true;press(1);assert(oledDetail&&installs==0);press(4);assert(!oledDetail&&navLevel==3);
 press(1);candidate=false;press(1);assert(installs==0);candidate=true;oledDetail=false;press(1);press(1);assert(installs==1);
 press(8);press(1);assert(cancels==1);
 uploadActive=true;press(4);assert(navLevel==3);uploadActive=false;
 // Slot start also requires two explicit presses and can be cancelled.
 oledOtaChoice=3;press(1);assert(oledPage==10);press(1);assert(moduleStarts==0);
 moduleAvailable=true;press(1);assert(oledDetail);press(4);assert(!oledDetail&&oledPage==10);
 press(1);press(1);assert(moduleStarts==1);press(4);assert(oledPage==2);press(4);assert(navLevel==0);
 oledMenuChoice=0;press(2);assert(oledMenuChoice==2);press(8);assert(oledMenuChoice==0);
 puts("NAVIGATION_TREE_OK: hierarchy, fixed button roles, expiry and explicit install confirmation");
}
