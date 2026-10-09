#include <cstdint>
#include <cstdio>
#include <string>
#include <type_traits>
#include <cassert>
#include <map>
int64_t mockTimer=0;
uint32_t mockMillis=0;
uint32_t millis(){return mockMillis;}
class String {
 std::string value;
 public:
 String()=default;String(const char*s):value(s){}String(std::string s):value(s){}
 template<typename T,typename=std::enable_if_t<std::is_arithmetic_v<T>>>String(T x):value(std::to_string(x)){}
 String&operator+=(const String&s){value+=s.value;return *this;}
 friend String operator+(String a,const String&b){return String(a.value+b.value);}
};
String telemetryBootId="boot";
struct Partition {size_t size=1310720;}partition;
const Partition *esp_ota_get_next_update_partition(void*){return &partition;}
struct EspMock{int checks=0;size_t getSketchSize(){++checks;mockTimer+=250000;return 1240000;}}ESP;
struct cJSON{std::map<std::string,double> values;};
cJSON* cJSON_AddNumberToObject(cJSON*o,const char*k,double v){o->values[k]=v;return o;}
#include "../firmware/diagnostics.h"
int main(){
 initializeDiagnosticFlash();assert(ESP.checks==1&&diagnosticMemory.otaFree==70720&&diagnosticFlashInitUs>=250000);
 initializeDiagnosticFlash();assert(ESP.checks==1);
 sampleDiagnostics();assert(diagnosticMemory.ready&&diagnosticOps[DIAG_SAMPLE].calls==1&&diagnosticOps[DIAG_SAMPLE].maxUs<1000);
 mockMillis=4999;sampleDiagnostics();assert(diagnosticOps[DIAG_SAMPLE].calls==1);
 mockMillis=5000;sampleDiagnostics();assert(diagnosticOps[DIAG_SAMPLE].calls==2&&ESP.checks==1);
 // Subsequent sampling does not rescan the firmware, including timer wraparound.
 diagnosticNextSample=UINT32_MAX-2;mockMillis=3;sampleDiagnostics();assert(ESP.checks==1&&diagnosticOps[DIAG_SAMPLE].calls==3);
 {DiagnosticScope scope(DIAG_RPC);scope.failed=true;mockTimer+=400000;}
 assert(diagnosticOps[DIAG_RPC].failures==1&&diagnosticOps[DIAG_RPC].maxUs>=400000);
 diagnosticLoopTick();mockTimer+=2000;diagnosticLoopTick();assert(diagnosticLoopMaxUs>=2000);
 (void)diagnosticText();(void)diagnosticUptime();cJSON obj;assert(diagnosticJSON(&obj));
 assert(obj.values["diag_sample_max_us"]<1000&&obj.values["diag_flash_init_us"]>=250000);
 puts("DIAGNOSTIC_SAMPLER_OK: one flash scan per boot, cached margin, 5s cadence, timer wrap, failure counters");
}
