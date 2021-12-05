#include "memory_cache.h"

memory_cache::memory_cache() {
    current_id = 0;
}

memory_cache::memory_cache(int cache_size, int number_of_quantums) {
    current_id = 0;
    cache_indexes.resize(cache_size);
    memory.resize(number_of_quantums);
}

memory_cache& memory_cache::operator=(const memory_cache& cache) {
    if (this != &cache) {
        current_id = cache.current_id;
        cache_indexes.resize(cache.cache_indexes.size());
        memory.resize(cache.memory.size());
        for (int i = 0; i < static_cast<int>(cache_indexes.size()); ++i) {
            cache_indexes[i] = cache.cache_indexes[i];
        }
        for (int i = 0; i < static_cast<int>(memory.size()); ++i) {
            memory[i] = cache.memory[i];
        }
    }
    return *this;
}

memory_cache& memory_cache::operator=(memory_cache&& cache) {
    if (this != &cache) {
        current_id = cache.current_id;
        memory = cache.memory;
        cache_indexes = cache.cache_indexes;

        // cache.cache_indexes = std::vector<int>();
        // cache.memory = std::vector<char*>();

    }
}

std::pair<char*, int> memory_cache::add(char* quantum, int quantum_index) {
    std::pair<char*, int> return_value = { nullptr, -1 };

    if (memory[quantum_index] != nullptr) {
        return return_value;
    }

    if (memory[cache_indexes[current_id]] != nullptr) {
        return_value = { memory[cache_indexes[current_id]], cache_indexes[current_id] };
        memory[cache_indexes[current_id]] = nullptr;
    }

    cache_indexes[current_id] = quantum_index;
    memory[quantum_index] = quantum;

    current_id = (current_id == static_cast<int>(cache_indexes.size()) - 1) ? 0 : current_id + 1;

    return return_value;
}

char* memory_cache::get(int quantum_index) {
    return memory[quantum_index];
}

bool memory_cache::is_contain(int quantum_index) {
    return (memory[quantum_index] != nullptr);
}
