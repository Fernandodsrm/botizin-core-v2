#pragma once
#include <cstdint>
extern int64_t mockTimer;
inline int64_t esp_timer_get_time(){mockTimer+=10;return mockTimer;}
