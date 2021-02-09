#ifndef __MEMORY_ALLOCATOR_H__
#define __MEMORY_ALLOCATOR_H__

#include <vector>
#include <queue>
#include <mutex>
#include "common.h"

class memory_allocator {
    int st = 1;
    int quantum_size;
    std::vector<int*> memory {};  // структура для хранения указателей на кванты памяти
    std::queue<int*> free_quantums {};  // очередь свободных квантов
    std::mutex lock;
public:
    int* alloc();
    void free(int** quantum);
    void set_quantum_size(int size_quantum);
    ~memory_allocator();
private:
    void resize_internal();
};

#endif  // __MEMORY_ALLOCATOR_H__
