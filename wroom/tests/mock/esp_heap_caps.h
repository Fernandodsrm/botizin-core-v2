#pragma once
#define MALLOC_CAP_INTERNAL 1
#define MALLOC_CAP_8BIT 2
#define MALLOC_CAP_SPIRAM 4
inline unsigned heap_caps_get_free_size(unsigned caps){return caps==4?0:100000;}
inline unsigned heap_caps_get_minimum_free_size(unsigned){return 90000;}
inline unsigned heap_caps_get_largest_free_block(unsigned){return 60000;}
