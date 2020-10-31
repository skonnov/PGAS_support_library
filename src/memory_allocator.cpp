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
    int teq = memory.size();
    memory.push_back(st*QUANTUM_SIZE);
    for(int i = 0; i < st; i++) {
        free_quantums.push(memory.data()+teq);
        teq += QUANTUM_SIZE;
    }
    st *= 2;
}
