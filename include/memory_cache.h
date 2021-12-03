#ifndef __MEMORY_CACHE__
#define __MEMORY_CACHE__

#include "memory_allocator.h"

class memory_cache {
public:
    void add(void* quantum);
    void* remove();
private:
    memory_allocator cache;
    int current_id;
};

#endif