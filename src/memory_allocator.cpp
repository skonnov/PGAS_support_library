#include "memory_allocator.h"

int* memory_allocator::alloc() {
    if (free_quantums.empty())
        resize_internal();
    int* quantum = free_quantums.front();
    free_quantums.pop();
    return quantum;
}

void memory_allocator::free(int** quantum) {
    free_quantums.push(*quantum);
    quantum = nullptr;
}

void memory_allocator::resize_internal() {
    memory.emplace_back(new int[st*quantum_size]);
    for(int i = 0; i < st; i++) {
        free_quantums.push(memory.back()+i*quantum_size);
    }
    st *= 2;
}

void memory_allocator::set_quantum_size(int size_quantum) {
    quantum_size = size_quantum;
}

memory_allocator::~memory_allocator() {
    for(int* i: memory) {
        delete[] i;
    }
}
