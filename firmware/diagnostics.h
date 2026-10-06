#pragma once
#include <esp_heap_caps.h>
#include <esp_timer.h>
// Fixed-size counters; no trace log, NVS write, task creation or network here.
struct DiagnosticOp { uint32_t calls=0, lastUs=0, maxUs=0, failures=0; uint64_t totalUs=0; };
enum DiagnosticIndex { DIAG_GIT, DIAG_TB, DIAG_RPC, DIAG_PEER, DIAG_OLED, DIAG_SAMPLE, DIAG_COUNT };
static DiagnosticOp diagnosticOps[DIAG_COUNT];
struct DiagnosticMemory { uint32_t free=0, minimum=0, largest=0, psram=0, otaFree=0, at=0; bool ready=false; };
static DiagnosticMemory diagnosticMemory;
static uint32_t diagnosticLoopAt=0, diagnosticLoopMaxUs=0, diagnosticNextSample=0;
static bool diagnosticLoopSeen=false;
struct DiagnosticScope {
  DiagnosticIndex index; uint32_t started; bool failed=false;
  explicit DiagnosticScope(DiagnosticIndex i):index(i),started((uint32_t)esp_timer_get_time()){}
  ~DiagnosticScope(){auto &o=diagnosticOps[index]; uint32_t elapsed=(uint32_t)esp_timer_get_time()-started;
    ++o.calls;o.lastUs=elapsed;if(elapsed>o.maxUs)o.maxUs=elapsed;o.totalUs+=elapsed;if(failed)++o.failures;}
};
static void diagnosticLoopTick(){uint32_t now=(uint32_t)esp_timer_get_time();
  if(diagnosticLoopSeen){uint32_t gap=now-diagnosticLoopAt;if(gap>diagnosticLoopMaxUs)diagnosticLoopMaxUs=gap;}
  diagnosticLoopAt=now;diagnosticLoopSeen=true;
}
static void sampleDiagnostics(){
  if(diagnosticMemory.ready&&(int32_t)(millis()-diagnosticNextSample)<0)return;
  diagnosticNextSample=millis()+5000;DiagnosticScope scope(DIAG_SAMPLE);
  diagnosticMemory.free=heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
  diagnosticMemory.minimum=heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
  diagnosticMemory.largest=heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
  diagnosticMemory.psram=heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  const auto *p=esp_ota_get_next_update_partition(nullptr);size_t size=ESP.getSketchSize();
  diagnosticMemory.otaFree=p&&size<p->size?p->size-size:0;
  diagnosticMemory.at=millis();diagnosticMemory.ready=true;
}
static uint32_t diagnosticUptime(){return (uint32_t)(esp_timer_get_time()/1000000LL);}
static String diagnosticText(){String s="DIAG_SCHEMA: 1\n";
  s+="BOOT_ID: "+telemetryBootId+"\n";
  s+="DIAG_FREE: "+String(diagnosticMemory.free)+"\nDIAG_MIN: "+String(diagnosticMemory.minimum)+"\n";
  s+="DIAG_LARGEST: "+String(diagnosticMemory.largest)+"\nDIAG_PSRAM: "+String(diagnosticMemory.psram)+"\n";
  s+="DIAG_OTA_FREE: "+String(diagnosticMemory.otaFree)+"\nDIAG_SAMPLE_AGE: "+String((millis()-diagnosticMemory.at)/1000)+"\n";
  s+="DIAG_LOOP_MAX_US: "+String(diagnosticLoopMaxUs)+"\n";
  const char *names[]={"GIT","TB","RPC","PEER","OLED","SAMPLE"};
  for(uint8_t i=0;i<DIAG_COUNT;++i){s+="DIAG_"+String(names[i])+"_MAX_US: "+String(diagnosticOps[i].maxUs)+"\n";}
  return s;
}
static bool diagnosticJSON(cJSON *o){
  bool ok=cJSON_AddNumberToObject(o,"diag_schema",1)&&
    cJSON_AddNumberToObject(o,"ram_internal_free_bytes",diagnosticMemory.free)&&
    cJSON_AddNumberToObject(o,"ram_internal_min_bytes",diagnosticMemory.minimum)&&
    cJSON_AddNumberToObject(o,"ram_largest_block_bytes",diagnosticMemory.largest)&&
    cJSON_AddNumberToObject(o,"psram_free_bytes",diagnosticMemory.psram)&&
    cJSON_AddNumberToObject(o,"ota_free_bytes",diagnosticMemory.otaFree)&&
    cJSON_AddNumberToObject(o,"diag_sample_age_seconds",(millis()-diagnosticMemory.at)/1000)&&
    cJSON_AddNumberToObject(o,"loop_max_gap_us",diagnosticLoopMaxUs);
  const char *names[]={"git","tb","rpc","peer","oled","sample"};char key[40];
  for(uint8_t i=0;i<DIAG_COUNT;++i){auto &d=diagnosticOps[i];
    snprintf(key,sizeof(key),"diag_%s_max_us",names[i]);ok=ok&&cJSON_AddNumberToObject(o,key,d.maxUs);
    snprintf(key,sizeof(key),"diag_%s_last_us",names[i]);ok=ok&&cJSON_AddNumberToObject(o,key,d.lastUs);
    snprintf(key,sizeof(key),"diag_%s_calls",names[i]);ok=ok&&cJSON_AddNumberToObject(o,key,d.calls);
    snprintf(key,sizeof(key),"diag_%s_failures",names[i]);ok=ok&&cJSON_AddNumberToObject(o,key,d.failures);
  }return ok;
}
