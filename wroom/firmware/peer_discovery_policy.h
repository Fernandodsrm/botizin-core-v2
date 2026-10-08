#pragma once
#include <stdint.h>
struct PeerDiscoveryPolicy {
  bool cached=false;
  uint32_t nextAttempt=0;
  bool due(uint32_t now) const {return !cached && (int32_t)(now-nextAttempt)>=0;}
  void attempted(uint32_t now,bool found) {nextAttempt=now+30000;cached=found;}
  void invalidate() {cached=false;}
  void reset(uint32_t now) {cached=false;nextAttempt=now;}
};
