#pragma once
// Tiny S3/XiaoZhi voice-state overlay for the WROOM face. No audio, tasks,
// networking, flash writes, dynamic allocation, or persistent conversations.
#include <atomic>
#include <stdint.h>

struct BotizinVoiceSync {
  std::atomic<uint8_t> lastState{0};
  std::atomic<uint32_t> lastAtMs{0};
  std::atomic<uint32_t> accepted{0};
  static constexpr uint32_t kTtlMs=4000;
  static bool valid(uint8_t state) {
    // Native XiaoZhi device states: idle, connecting, listening, speaking.
    return state==3 || state==4 || state==5 || state==6;
  }
  bool receive(uint8_t state,uint32_t now){
    if(!valid(state))return false;
    lastState.store(state,std::memory_order_relaxed);
    lastAtMs.store(now,std::memory_order_release);
    accepted.fetch_add(1,std::memory_order_relaxed);
    return true;
  }
  uint8_t mode(uint32_t now) const {
    if(accepted.load(std::memory_order_relaxed)==0)return 0;
    const uint32_t age=uint32_t(now-lastAtMs.load(std::memory_order_acquire));
    if(age>kTtlMs)return 0; // short fail-safe; normal randomized face resumes
    return lastState.load(std::memory_order_relaxed);
  }
  uint32_t age(uint32_t now) const {
    if(accepted.load(std::memory_order_relaxed)==0)return 0;
    return uint32_t(now-lastAtMs.load(std::memory_order_acquire));
  }
};
