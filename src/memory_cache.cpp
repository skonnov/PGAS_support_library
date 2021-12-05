#include "memory_cache.h"

memory_cache::memory_cache(int size_quantum, int size_of, int cache_size, int number_of_quantums) {
    current_id = 0;
    quantum_size = size_quantum * size_of;
    cache_indexes.resize(cache_size);
    memory.resize(number_of_quantums);
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
