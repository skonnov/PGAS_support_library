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
    int add(int quantum_index);  // добавить элемент в кеш. Если при попытке добавления
                                 // происходит замещение другого элемента, возвращается
                                 // индекс замещаемого элемента
    bool is_contain(int quantum_index);
    void add_to_excluded(int quantum_index);
    bool is_excluded(int quantum_index);
    void clear_cache();
private:
    std::vector<bool> excluded {};
    std::vector<bool> contain_flags {};
    std::vector<int> cache_indexes {};
    int current_id;
};

#endif
