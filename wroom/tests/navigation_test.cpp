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
uint32_t millis(){return 1000;}
void pinMode(uint8_t,int){}
void attachInterruptArg(uint8_t,void(*)(void*),void*,int){}
struct SerialMock{void println(const char*){} template<typename... T>void printf(const char*,T...){} }Serial;
bool uploadActive=false,uploadOK=false,rebootScheduled=false,otaCheckQueued=false,otaInstallQueued=false;
bool oledMenu=true,oledDetail=false;
uint8_t oledMenuChoice=0,oledOtaChoice=0,oledPage=0;
uint32_t nextPeerPoll=0,nextOledRefresh=0;
const char *otaCandidateId="id",*otaTargetVersion="0.0.9",*otaTargetSHA="sha",*telemetryBootId="boot";
struct QueueMock{bool empty=true;bool isEmpty()const{return empty;}}s3QueuedAction;
uint8_t oledS3Choice=0;bool s3Candidate=false;int s3checks=0,s3installs=0,s3cancels=0,s3stages=0;
bool s3CandidateReady(){return s3Candidate;}
void stageS3Confirmation(){++s3stages;}
bool queueS3Action(const char *action){
 if(action[0]=='c'&&action[1]=='h')++s3checks;
 else if(action[0]=='c'&&action[1]=='o')++s3installs;
 else ++s3cancels;
 return true;
}
bool candidate=false,window=false;int installs=0,checks=0,cancels=0;
bool candidateReady(){return candidate;}
bool manualWindowActive(){return window;}
bool queueOtaCheck(){++checks;return true;}
bool confirmOta(const char*,const char*,const char*,const char*){++installs;return true;}
void cancelOtaCheck(){++cancels;candidate=false;}
#include "../firmware/navigation.h"
void press(uint8_t p){navPending=p;pollNavigation();}
int main(){
 beginNavigation();discardNavigation();
 press(8);assert(oledMenuChoice==1&&oledMenu);press(1);assert(oledPage==0&&!oledMenu);
 press(4);assert(oledMenu);press(8);press(1);assert(oledPage==1&&!oledMenu&&nextPeerPoll==1000);
 press(8);assert(oledPage==2);press(1);assert(checks==1&&installs==0);
 press(8);press(1);assert(!oledDetail&&installs==0); // disabled without a candidate
 candidate=true;window=true;press(1);assert(oledDetail&&installs==0); // opens confirmation only
 press(4);assert(!oledDetail&&!oledMenu&&installs==0&&cancels==0); // NO returns, no install
 press(1);candidate=false;press(1);assert(installs==0); // expired candidate cannot install
 candidate=true;oledDetail=false;press(1);press(1);assert(installs==1); // explicit second OK
 oledDetail=false;press(8);press(1);assert(cancels==1); // separate cancel action
 uploadActive=true;auto page=oledPage;press(4);assert(oledPage==page&&!oledMenu);uploadActive=false;
 press(4);assert(oledMenu);press(4);assert(oledMenu); // no side effect at home
 oledMenuChoice=2;press(8);assert(oledMenuChoice==3);press(8);assert(oledMenuChoice==4);press(1);assert(oledPage==5&&!oledMenu);
 press(1);assert(s3checks==1);press(8);press(1);assert(s3installs==0&&!oledDetail);
 s3Candidate=true;press(1);assert(s3stages==1&&s3installs==0&&oledDetail);
 press(4);assert(!oledDetail&&!oledMenu&&s3installs==0);
 press(1);s3Candidate=false;press(1);assert(s3installs==0);
 s3Candidate=true;oledDetail=false;press(1);press(1);assert(s3installs==1);
 press(8);press(1);assert(s3cancels==1);
 press(4);press(8);assert(oledMenuChoice==5);press(8);assert(oledMenuChoice==0);
 press(2);assert(oledMenuChoice==5);s3QueuedAction.empty=false;press(1);assert(oledMenu);s3QueuedAction.empty=true;
 puts("NAVIGATION_TESTS_OK: menu, disabled install, explicit confirmation, cancel, expired candidate, busy guard");
}
