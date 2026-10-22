#include "../firmware/ota_boot_health.h"
#include <cassert>
#include <cstdio>
static void e(OtaHealthDecision wanted,bool pending,bool partitions,bool wifi,bool http,
              uint32_t elapsed,uint32_t healthy,const char* label) {
  const auto result=otaHealthDecision(pending,partitions,wifi,http,elapsed,healthy);
  if(result!=wanted) {printf("FAIL %s expected=%d got=%d\n",label,(int)wanted,(int)result);assert(false);}
  printf("PASS %s\n",label);
}
int main(){
  using D=OtaHealthDecision;
  e(D::NOT_PENDING,false,true,true,true,10000,10000,"already valid");
  e(D::WAIT,true,true,false,false,0,0,"cold boot waiting");
  e(D::WAIT,true,true,true,true,14999,14999,"not stable 15 seconds");
  e(D::CONFIRM,true,true,true,true,15000,15000,"stable local Wi-Fi and HTTP");
  e(D::WAIT,true,true,false,true,70000,0,"wifi disconnect resets streak");
  e(D::WAIT,true,true,true,false,70000,0,"HTTP unavailable");
  e(D::CONFIRM,true,true,true,true,50000,15000,"MQTT broker outage irrelevant");
  e(D::CONFIRM,true,true,true,true,60000,30000,"time/NTP outage irrelevant");
  e(D::WAIT,true,true,true,true,170000,0,"near deadline not yet good");
  e(D::ROLLBACK,true,true,false,false,180000,0,"no wifi timeout");
  e(D::ROLLBACK,true,true,true,false,180000,0,"HTTP down timeout");
  e(D::ROLLBACK,true,false,false,false,1000,0,"invalid partitions immediate rollback");
  e(D::ROLLBACK,true,true,true,true,180000,15000,"timeout has priority");
  e(D::WAIT,true,true,true,true,16000,14999,"streak reset too soon");
  e(D::ROLLBACK,true,true,false,false,UINT32_MAX,0,"long elapsed");
  puts("OTA04_LOCAL_POLICY_TESTS_OK 15 cases");
}
