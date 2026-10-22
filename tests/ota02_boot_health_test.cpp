#include "../firmware/ota_boot_health.h"
#include <cassert>
#include <cstdio>

static void expect(OtaHealthDecision expected, bool pending, bool flash,
                   bool wifi, bool ntp, bool http, bool mqtt,
                   uint32_t elapsed, uint32_t healthy, const char* label) {
    auto actual=otaHealthDecision(pending,flash,wifi,ntp,http,mqtt,elapsed,healthy);
    if (actual!=expected) {
        printf("FAIL %s expected=%d actual=%d\n",label,(int)expected,(int)actual);
        assert(false);
    }
    printf("PASS %s\n", label);
}
int main() {
    using A=OtaHealthDecision;
    expect(A::NOT_PENDING,false,true,true,true,true,true,400000,300000,"already valid/no rollback");
    expect(A::WAIT,true,true,false,false,false,false,0,0,"cold boot pending");
    expect(A::WAIT,true,true,true,true,true,true,2000,2000,"too early to confirm");
    expect(A::WAIT,true,true,true,true,true,true,20000,14999,"continuous health below threshold");
    expect(A::CONFIRM,true,true,true,true,true,true,35000,15000,"15 seconds good health");
    expect(A::WAIT,true,true,false,true,true,true,45000,0,"wifi dropped, reset healthy streak");
    expect(A::WAIT,true,true,true,false,true,true,46000,0,"clock not synchronized");
    expect(A::WAIT,true,true,true,true,false,true,46000,0,"web server not started");
    expect(A::WAIT,true,true,true,true,true,false,46000,0,"no MQTT publish");
    expect(A::ROLLBACK,true,true,false,false,false,false,180000,0,"timeout no network");
    expect(A::ROLLBACK,true,true,true,true,true,false,180000,0,"timeout broker outage");
    expect(A::ROLLBACK,true,true,true,true,true,true,180000,80000,"timeout wins even if healthy");
    expect(A::ROLLBACK,true,false,true,true,true,true,3000,3000,"invalid partitions rollback promptly");
    expect(A::ROLLBACK,true,true,false,true,true,true,UINT32_MAX,0,"elapsed wrap-safe subtraction");
    expect(A::CONFIRM,true,true,true,true,true,true,20000,15000,"normal successful candidate");
    puts("OTA02_POLICY_TESTS_OK 15 cases");
    return 0;
}
