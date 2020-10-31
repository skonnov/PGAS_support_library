#ifndef __MEMORY_ALLOCATOR_H__
#define __MEMORY_ALLOCATOR_H__

#include <vector>
#include <queue>

class memory_allocator {
    int st = 1;
    std::vector<int> memory;
    std::queue<int*> free_quantums;
public:
    int* alloc();
    void free(int* quantum);
private:
    void resize_internal();
};

#endif  // __MEMORY_ALLOCATOR_H__
