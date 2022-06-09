#include "memory_allocator.h"

char* memory_allocator::alloc() {
    const std::lock_guard<std::mutex> lockg(lock);
    if (free_quantums.empty())  // если свободные кванты закончились, создаются новые
        resize_internal();
    char* quantum = free_quantums.front();
    free_quantums.pop();
    return quantum;
}

void memory_allocator::free(char** quantum) {
    const std::lock_guard<std::mutex> lockg(lock);
    free_quantums.push(*quantum);
    *quantum = nullptr;
}

void memory_allocator::resize_internal() {
    memory.emplace_back(new char[st * quantum_size]);
    for (int i = 0; i < st; ++i) {
        free_quantums.push(memory.back() + i * quantum_size);
    }
    st *= 2;
}

void memory_allocator::set_quantum_size(int size_quantum, int size_of) {
    quantum_size = size_quantum * size_of;
}

memory_allocator::~memory_allocator() {
    for(auto* i: memory) {
        delete[] i;
    }
}
