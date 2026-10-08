#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>
using String = std::string;
constexpr int ESP_OK = 0, ESP_TIMER_TASK = 0;
struct esp_partition_t { uint32_t address; size_t size; };
using esp_timer_handle_t = void *;
struct esp_timer_create_args_t { void (*callback)(void *); int dispatch_method; const char *name; };
static esp_partition_t a{0x10000,0x140000}, b{0x150000,0x140000};
static const esp_partition_t *running=&a, *previous=&b, *boot=&a;
static bool goodHash=true, readable=true, badReadback=false;
static int createResult=0, startResult=0, bootResult=0;
static int bootCalls=0, stops=0, deletes=0, restarts=0;
static uint64_t timerMicros=0;
static void (*savedCallback)(void *)=nullptr;
const esp_partition_t *esp_ota_get_running_partition() { return running; }
const esp_partition_t *esp_ota_get_next_update_partition(void *) { return previous; }
const esp_partition_t *esp_ota_get_boot_partition() { return badReadback ? running : boot; }
int esp_ota_set_boot_partition(const esp_partition_t *p) { ++bootCalls; if (!bootResult) boot=p; return bootResult; }
int esp_timer_create(const esp_timer_create_args_t *args, esp_timer_handle_t *h) {
  savedCallback=args->callback; *h=reinterpret_cast<void *>(1); return createResult;
}
int esp_timer_start_once(esp_timer_handle_t, uint64_t us) { timerMicros=us; return startResult; }
int esp_timer_stop(esp_timer_handle_t) { ++stops; return 0; }
int esp_timer_delete(esp_timer_handle_t) { ++deletes; return 0; }
void esp_restart() { ++restarts; }
bool hashPartition(const esp_partition_t *, size_t n, String &result) {
  assert(n==1248160);
  result=goodHash ? "11a0a1c68a8b7bd7c9c1bd7782df070ece556e2ff1eec94f3899e01e4c6aa2f5" : "bad";
  return readable;
}
static void trialTurnLedOff() {}
#define BOTIZIN_TRIAL_HOST_TEST
#include "../menu-trial/timed_trial.h"
void reset() {
  running=&a; previous=&b; boot=&a; a={0x10000,0x140000}; b={0x150000,0x140000};
  goodHash=readable=true; badReadback=false;
  createResult=startResult=bootResult=bootCalls=stops=deletes=restarts=0;
  timerMicros=0; savedCallback=nullptr; trialArmed=false; trialTimer=nullptr;
}
int main() {
  reset(); assert(armTimedTrial()); assert(boot==previous && trialArmed && timerMicros==600000000);
  savedCallback(nullptr); assert(restarts==1);
  reset(); running=&b; previous=&a; boot=&b; assert(armTimedTrial() && boot==&a);
  reset(); goodHash=false; assert(!armTimedTrial() && bootCalls==0);
  reset(); readable=false; assert(!armTimedTrial() && bootCalls==0);
  reset(); previous=nullptr; assert(!armTimedTrial() && bootCalls==0);
  reset(); previous=running; assert(!armTimedTrial() && bootCalls==0);
  reset(); b.size=0x200000; assert(!armTimedTrial() && bootCalls==0);
  reset(); b.address=0x410000; assert(!armTimedTrial() && bootCalls==0);
  reset(); createResult=1; assert(!armTimedTrial() && bootCalls==0);
  reset(); startResult=1; assert(!armTimedTrial() && bootCalls==0 && deletes==1);
  reset(); bootResult=1; assert(!armTimedTrial() && !trialArmed && stops==1 && deletes==1);
  reset(); badReadback=true; assert(!armTimedTrial() && !trialArmed && stops==1);
}
