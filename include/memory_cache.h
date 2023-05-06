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
#include "statistic.h"

enum cache_statistic_operations {
    PUT_IN_CACHE = 0,
    REMOVE_FROM_CACHE = 1,
    ADD_TO_EXCLUDED = 2,
    REMOVE_FROM_EXCLUDED = 3,
    ALREADY_IN_CACHE = 4
};

class memory_cache {
public:
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
    void init(int cache_size, int number_of_quantums, MPI_Comm comm, int key, const std::string& statistics_output_directory, statistic* stat = nullptr);
private:
    std::string statistics_output_directory;
    std::vector<bool> excluded {};
    std::vector<cache_node*> contain_flags {};
    std::vector<cache_node> cache_memory {};
    cache_list free_cache_nodes {}, cache_indexes {};
    int rank, size;
    MPI_Comm workers_comm;
    int key;
    statistic* stat;
    const std::vector<std::vector<quantum_cluster_info>>* vector_quantum_cluster_info;
#if (ENABLE_STATISTICS_COLLECTION)
    int cache_miss_cnt = 0, cache_miss_cnt_no_free = 0;
    static std::ofstream statistic_file_stream;
    std::ofstream cache_miss_cnt_file_stream;
#endif
};

#endif
