#ifndef __MEMORY_ALLOCATOR_H__
#define __MEMORY_ALLOCATOR_H__

#include <vector>
#include <queue>
#include <mutex>
#include "common.h"

class memory_allocator {
    int st = 1;
    int quantum_size;
    std::vector<char*> memory {};  // структура для хранения указателей на кванты памяти
    std::queue<char*> free_quantums {};  // очередь свободных квантов
    std::mutex lock;
public:
    char* alloc();
    void free(char** quantum);
    void set_quantum_size(int size_quantum, int size_of);
    ~memory_allocator();
private:
    void resize_internal();
};

#endif  // __MEMORY_ALLOCATOR_H__
