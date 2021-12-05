#ifndef __MEMORY_CACHE__
#define __MEMORY_CACHE__

#include "memory_allocator.h"
#include <utility>

class memory_cache {
public:
    memory_cache();
    memory_cache(int cache_size, int number_of_quantums);
    memory_cache& operator=(const memory_cache& cache);
    memory_cache& operator=(memory_cache&& cache);
    std::pair<char*, int> add(char* quantum, int quantum_index);
    bool is_contain(int quantum_index);
    char* get(int quantum_index);
private:
    std::vector<char*> memory {};
    std::vector<int> cache_indexes {};
    int current_id;
};

#endif
