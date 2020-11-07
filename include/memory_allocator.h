#ifndef __MEMORY_ALLOCATOR_H__
#define __MEMORY_ALLOCATOR_H__

#include <vector>
#include <queue>
#include "common.h"

class memory_allocator {
    int st = 1;
    int quantum_size;
    std::vector<int*> memory {};
    std::queue<int*> free_quantums {};
public:
    int* alloc();
    void free(int** quantum);
    void set_quantum_size(int size_quantum);  // rewrite it w/ using constructor?
    ~memory_allocator();
private:
    void resize_internal();
};

#endif  // __MEMORY_ALLOCATOR_H__
