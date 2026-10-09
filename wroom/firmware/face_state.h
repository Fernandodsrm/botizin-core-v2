#pragma once
#include <atomic>
#include <stdint.h>
// Shared mode/counters only. The renderer never reads network Strings or OTA flags.
static std::atomic<bool> faceActive{true};
static std::atomic<bool> faceDisplayEnabled{false};
static std::atomic<uint32_t> faceFrames{0};
static std::atomic<uint32_t> faceMaxGapMs{0};
static uint32_t faceLastActivity=0;
static bool faceIdleDue(uint32_t now,uint32_t last,bool busy){
  return !busy && uint32_t(now-last)>=30000;
}
static inline void pauseFace(){faceActive.store(false);faceLastActivity=millis();}
