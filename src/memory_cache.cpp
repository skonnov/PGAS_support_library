#include "memory_cache.h"

memory_cache::memory_cache() {
    current_id = 0;
}

memory_cache::memory_cache(int cache_size, int number_of_quantums) {
    current_id = 0;
    cache_indexes.resize(cache_size, -1);
    contain_flags.resize(number_of_quantums);
}

memory_cache& memory_cache::operator=(const memory_cache& cache) {
    if (this != &cache) {
        current_id = cache.current_id;
        cache_indexes.resize(cache.cache_indexes.size());
        for (int i = 0; i < static_cast<int>(cache_indexes.size()); ++i) {
            cache_indexes[i] = cache.cache_indexes[i];
        }
        contain_flags.resize(cache.contain_flags.size());
        for (int i = 0; i < static_cast<int>(contain_flags.size()); ++i) {
            contain_flags[i] = cache.contain_flags[i];
        }
    }
    return *this;
}

memory_cache& memory_cache::operator=(memory_cache&& cache) {
    if (this != &cache) {
        current_id = cache.current_id;
        cache_indexes = std::move(cache.cache_indexes);
        contain_flags = std::move(cache.contain_flags);
    }
    return *this;
}

int memory_cache::add(int quantum_index) {
    int return_index = -1;

    if (is_contain(quantum_index)) {
        return return_index;
    }

    if (cache_indexes[current_id] >= 0) {
        return_index = cache_indexes[current_id];
    }

    cache_indexes[current_id] = quantum_index;
    current_id = (current_id == static_cast<int>(cache_indexes.size()) - 1) ? 0 : current_id + 1;
    return return_index;
}

bool memory_cache::is_contain(int quantum_index) {
    return contain_flags[quantum_index];
}

void memory_cache::add_to_excluded(int quantum_index) {
    excluded[quantum_index] = true;
}

bool memory_cache::is_excluded(int quantum_index) {
    return excluded[quantum_index];
}

void memory_cache::clear_cache() {
    current_id = 0;
    cache_indexes = std::vector<int>(cache_indexes.size(), -1);
    excluded = std::vector<bool>(excluded.size(), -1);
    contain_flags = std::vector<bool>(excluded.size(), -1);
}