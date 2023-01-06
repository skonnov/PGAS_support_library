#ifndef __MEMORY_CACHE__
#define __MEMORY_CACHE__

#include <mpi.h>
#include <vector>
#include <utility>
#include <queue>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <string>
#include <mutex>
#include "common.h"
#include "cache_list.h"

enum cache_statistic_operations {
    PUT_IN_CACHE = 0,
    REMOVE_FROM_CACHE = 1,
    ADD_TO_EXCLUDED = 2,
    REMOVE_FROM_EXCLUDED = 3,
    ALREADY_IN_CACHE = 4
};

class memory_cache {
public:
    memory_cache(int key=0);
    memory_cache(int cache_size, int number_of_quantums, MPI_Comm workers_comm, int key=0);
    memory_cache& operator=(const memory_cache& cache);
    memory_cache& operator=(memory_cache&& cache);
    int add(int quantum_index);  // добавить элемент в кеш. Если при попытке добавления
                                 // происходит замещение другого элемента, возвращается
                                 // индекс замещаемого элемента
    bool is_contain(int quantum_index);
    void add_to_excluded(int quantum_index);
    bool is_excluded(int quantum_index);
    void delete_elem(int quantum_index);
    void get_cache_miss_cnt_statistics(int key, int number_of_elements);
    void update(int quantum_index);
private:
    std::vector<bool> excluded {};
    std::vector<cache_node*> contain_flags {};
    std::vector<cache_node> cache_memory {};
    cache_list free_cache_nodes {}, cache_indexes {};
    int rank, size;
    MPI_Comm workers_comm;
    int key;
#if (ENABLE_STATISTICS_COLLECTION)
    int cache_miss_cnt = 0, cache_miss_cnt_no_free = 0;
    std::ofstream statistic_file_stream;
    FILE* statistic_file;
    std::ofstream cache_miss_cnt_file_stream;
    std::mutex mt;
    int cnt = 0;
#endif
};

#endif
