#ifndef __MEMORY_CACHE__
#define __MEMORY_CACHE__

#include <mpi.h>
#include <vector>
#include <utility>
#include <queue>
#include <iostream>
#include <fstream>
#include "common.h"

struct cache_node {
    int value;
    cache_node* next, *prev;
};

class cache_list {
    cache_node* begin = nullptr, *end = nullptr;
    int size = 0;
public:
    void push_back(cache_node* node);
    cache_node* pop_front();
    cache_node* pop_back();
    void delete_node(cache_node* node);
    bool empty();
};

class memory_cache {
public:
    memory_cache();
    memory_cache(int cache_size, int number_of_quantums);
    memory_cache& operator=(const memory_cache& cache);
    memory_cache& operator=(memory_cache&& cache);
    int add(int quantum_index);  // добавить элемент в кеш. Если при попытке добавления
                                 // происходит замещение другого элемента, возвращается
                                 // индекс замещаемого элемента
    bool is_contain(int quantum_index);
    void add_to_excluded(int quantum_index);
    bool is_excluded(int quantum_index);
    void delete_elem(int quantum_index);
    ~memory_cache();
private:
    std::vector<bool> excluded {};
    std::vector<cache_node*> contain_flags {};
    std::vector<cache_node> cache_memory {};
    cache_list free_cache_nodes {}, cache_indexes {};
    int rank, size;
#if (ENABLE_STATISTICS_COLLECTION)
    int cache_miss_cnt = 0;
    std::ofstream statistic_file_stream;
#endif
};

#endif
