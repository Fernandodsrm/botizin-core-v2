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
bool s3OtaRecent(){return true;}uint32_t s3WindowUntil=2000;
uint8_t oledS3Choice=0;bool s3Candidate=false;int s3checks=0,s3installs=0,s3cancels=0,s3stages=0;
bool s3CandidateReady(){return s3Candidate;}
void stageS3Confirmation(){++s3stages;}
bool queueS3Action(const char *action){
 if(action[0]=='c'&&action[1]=='h')++s3checks;
 else if(action[0]=='c'&&action[1]=='o')++s3installs;
 else ++s3cancels;
 return true;
}
bool moduleAvailable=false;const char *moduleSHA="module";int moduleStarts=0;
bool queueModuleStart(const char*,const char*){++moduleStarts;return true;}
bool candidate=false,window=false;int installs=0,checks=0,cancels=0;
bool candidateReady(){return candidate;}
bool manualWindowActive(){return window;}
bool queueOtaCheck(){++checks;return true;}
bool confirmOta(const char*,const char*,const char*,const char*){++installs;return true;}
void cancelOtaCheck(){++cancels;candidate=false;}
#include "../firmware/navigation_model.h"
#include "../firmware/navigation.h"
void press(uint8_t p){navPending=p;pollNavigation();}
int main(){
 beginNavigation();discardNavigation();
 // Root -> Connection -> WROOM; down cannot jump to another area.
 press(1);assert(navLevel==1);press(1);assert(navLevel==3&&oledPage==8&&navBoard==0);
 press(8);assert(navLevel==3&&oledPage==8&&navBoard==0);
 press(4);assert(navLevel==1);press(8);press(1);assert(navBoard==1&&nextPeerPoll==1000);
 press(4);press(4);assert(navLevel==0&&oledMenu);
 // Diagnostics requires board then category, and back preserves each selection.
 press(8);press(1);press(1);assert(navLevel==2);press(8);press(1);assert(navLevel==3&&oledPage==7&&navDiagnostic==1);
 press(8);assert(navDiagnostic==1&&oledPage==7);press(4);assert(navLevel==2);press(4);assert(navLevel==1);press(4);
 // WROOM query and explicit second confirmation.
 oledMenuChoice=2;press(1);navBoard=0;press(1);assert(oledPage==2);
 press(1);assert(checks==1);press(8);press(1);assert(installs==0&&!oledDetail);
 candidate=true;window=true;press(1);assert(oledDetail&&installs==0);press(4);assert(!oledDetail&&navLevel==3);
 press(1);candidate=false;press(1);assert(installs==0);candidate=true;oledDetail=false;press(1);press(1);assert(installs==1);
 press(8);press(1);assert(cancels==1);
 // S3 uses same hierarchy and install gate.
 press(4);navBoard=1;press(1);assert(oledPage==5);press(1);assert(s3checks==1);press(8);press(1);assert(s3installs==0);
 s3Candidate=true;press(1);assert(oledDetail&&s3stages==1);press(4);assert(!oledDetail&&navLevel==3);
 press(1);s3Candidate=false;press(1);assert(s3installs==0);s3Candidate=true;oledDetail=false;press(1);press(1);assert(s3installs==1);
 press(8);press(1);assert(s3cancels==1);
 uploadActive=true;press(4);assert(navLevel==3);uploadActive=false;press(4);press(4);
 assert(navLevel==0);
 oledMenuChoice=0;press(2);assert(oledMenuChoice==2);press(8);assert(oledMenuChoice==0);
 oledMenuChoice=2;press(1);navBoard=0;press(1);oledOtaChoice=3;press(1);assert(oledPage==10);press(1);assert(moduleStarts==0);moduleAvailable=true;press(1);assert(oledDetail);press(4);assert(!oledDetail&&oledPage==10);press(1);press(1);assert(moduleStarts==1);press(4);assert(oledPage==2);
 puts("NAVIGATION_TREE_OK: hierarchy, fixed button roles, expiry and explicit install confirmation");
}
