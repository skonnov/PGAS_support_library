#include "memory_cache.h"
#include "common.h"

void cache_list::push_back(cache_node* node) {
    CHECK(node != nullptr, ERR_NULLPTR);
    if (begin == nullptr && end == nullptr) {
        begin = end = node;
        node->next = node->prev = nullptr;
    } else if (begin == end) {
        begin->next = node;
        node->prev = begin;
        node->next = nullptr;
        end = node;
    } else {
        end->next = node;
        node->prev = end;
        node->next = nullptr;
        end = node;
    }
    ++size;
}

cache_node* cache_list::pop_front() {
    CHECK(begin != nullptr, ERR_NULLPTR);
    cache_node* return_value = begin;
    if (begin == end) {
        begin = end = nullptr;
    } else if (begin->next == end) {
        begin = end;
        begin->prev = begin->next = end->next = end->prev = nullptr;
    } else {
        CHECK(begin->next != nullptr, ERR_NULLPTR);
        begin = begin->next;
        begin->prev = nullptr;
    }
    --size;
    return return_value;
}

cache_node* cache_list::pop_back() {
    CHECK(end != nullptr, ERR_NULLPTR);
    cache_node* return_value = end;
    if (begin == end) {
        begin = end = nullptr;
    } else if (begin->next == end) {
        begin = end;
        begin->prev = begin->next = end->next = end->prev = nullptr;
    } else {
        CHECK(end->prev != nullptr, ERR_NULLPTR);
        end = end->prev;
        end->next = nullptr;
    }
    --size;
    return return_value;
}

void cache_list::delete_node(cache_node* node) {
    CHECK(node != nullptr, ERR_NULLPTR);
    CHECK(begin!= nullptr, ERR_NULLPTR);
    cache_node* next_node = node->next;
    if (node == begin) {
        pop_front();
    } else if (node == end) {
        pop_back();
    } else {
        if (node->prev) {
            node->prev->next = node->next;
        }
        if (node->next) {
            node->next->prev = node->prev;
        }
    }
}

bool cache_list::empty() {
    return begin == end && begin == nullptr;
}




memory_cache::memory_cache() {
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
#if (ENABLE_STATISTICS_COLLECTION)
  #if (ENABLE_STATISTICS_EVERY_CACHE_MISSES)
    statistic_file_stream.open(STATISTICS_OUTPUT_DIRECTORY + "memory_cache_" + std::to_string(rank) + ".txt");
  #endif
#endif
}

memory_cache::memory_cache(int cache_size, int number_of_quantums, MPI_Comm comm):
                                        cache_memory(cache_size, {-1, nullptr, nullptr}),
                                        contain_flags(number_of_quantums, nullptr),
                                        excluded(number_of_quantums, false),
                                        workers_comm(comm) {
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
#if (ENABLE_STATISTICS_COLLECTION)
  #if (ENABLE_STATISTICS_EVERY_CACHE_MISSES)
    statistic_file_stream.open(STATISTICS_OUTPUT_DIRECTORY + "memory_cache_" + std::to_string(rank) + ".txt");
  #endif
#endif
    for (int i = 0; i < cache_size; ++i) {
        free_cache_nodes.push_back(&cache_memory[i]);
    }
}

memory_cache& memory_cache::operator=(const memory_cache& cache) {
    if (this != &cache) {
        cache_memory.resize(cache.cache_memory.size());
        for (int i = 0; i < static_cast<int>(cache_memory.size()); ++i) {
            cache_memory[i] = cache.cache_memory[i];
        }
        contain_flags.resize(cache.contain_flags.size());
        for (int i = 0; i < static_cast<int>(contain_flags.size()); ++i) {
            contain_flags[i] = cache.contain_flags[i];
        }
        excluded.resize(cache.excluded.size());
        for (int i = 0; i < static_cast<int>(contain_flags.size()); ++i) {
            excluded[i] = cache.excluded[i];
        }
        free_cache_nodes = cache.free_cache_nodes;
        cache_indexes = cache.cache_indexes;
        workers_comm = cache.workers_comm;
#if (ENABLE_STATISTICS_COLLECTION)
    #if (ENABLE_STATISTICS_CACHE_MISSES_CNT)
        cache_miss_cnt = cache.cache_miss_cnt;
    #endif
    // statistic_file_stream?
#endif
    }
    return *this;
}

memory_cache& memory_cache::operator=(memory_cache&& cache) {
    if (this != &cache) {
        cache_memory = std::move(cache.cache_memory);
        contain_flags = std::move(cache.contain_flags);
        excluded = std::move(cache.excluded);
        free_cache_nodes = cache.free_cache_nodes;
        cache_indexes = cache.cache_indexes;
        workers_comm = cache.workers_comm;
        #if (ENABLE_STATISTICS_COLLECTION)
            #if (ENABLE_STATISTICS_CACHE_MISSES_CNT)
                cache_miss_cnt = cache.cache_miss_cnt;
            #endif
            #if (ENABLE_STATISTICS_EVERY_CACHE_MISSES)
                statistic_file_stream = std::move(cache.statistic_file_stream);
            #endif
        #endif
    }
    return *this;
}

int memory_cache::add(int quantum_index) {
    CHECK(quantum_index >= 0 && quantum_index < (int)contain_flags.size(), ERR_OUT_OF_BOUNDS);
    // элемент уже находится в кеше?
    if (is_contain(quantum_index)) {
        // Least recently used (LRU) cache logic
        cache_indexes.delete_node(contain_flags[quantum_index]);
        cache_indexes.push_back(contain_flags[quantum_index]);
        return -1;
    }
    // элемент находится в списке исключённых элементов?
    if (is_excluded(quantum_index)) {
        return -1;
    }

#if (ENABLE_STATISTICS_COLLECTION)
  #if (ENABLE_STATISTICS_CACHE_MISSES_CNT)
    ++cache_miss_cnt;
  #endif
  #if (ENABLE_STATISTICS_EVERY_CACHE_MISSES)
    std::string info = std::to_string(quantum_index) + " " + std::to_string(MPI_Wtime()) + "\n";
    statistic_file_stream << info;
    // PRINT_TO_FILE(statistic_output_directory, "memory_cache", info);
  #endif
#endif

    // список свободных элементов не пуст?
    if (!free_cache_nodes.empty()) {
        // взять элемент из списка свободных элементов
        cache_node* node = free_cache_nodes.pop_front();
        node->value = quantum_index;
        cache_indexes.push_back(node);
        contain_flags[quantum_index] = node;
        return -1;
    }

#if (ENABLE_STATISTICS_COLLECTION)
  #if (ENABLE_STATISTICS_CACHE_MISSES_CNT)
    ++cache_miss_cnt_no_free;
  #endif
#endif

    // вытеснение кванта из кеша текущим квантом
    cache_node* node = cache_indexes.pop_front();
    contain_flags[node->value] = nullptr;

    int return_value = node->value;

    node->value = quantum_index;
    contain_flags[quantum_index] = node;

    cache_indexes.push_back(node);

    return return_value;
}

bool memory_cache::is_contain(int quantum_index) {
    CHECK(quantum_index >= 0 && quantum_index < (int)contain_flags.size(), ERR_OUT_OF_BOUNDS);
    return contain_flags[quantum_index] != nullptr;
}

void memory_cache::add_to_excluded(int quantum_index) {
    delete_elem(quantum_index);
    excluded[quantum_index] = true;

}

bool memory_cache::is_excluded(int quantum_index) {
    CHECK(quantum_index >= 0 && quantum_index < (int)excluded.size(), ERR_OUT_OF_BOUNDS);
    return excluded[quantum_index];
}

void memory_cache::delete_elem(int quantum_index) {
    CHECK(quantum_index >= 0 && quantum_index < (int)excluded.size(), ERR_OUT_OF_BOUNDS);
    if (is_contain(quantum_index)) {
        cache_indexes.delete_node(contain_flags[quantum_index]);
        free_cache_nodes.push_back(contain_flags[quantum_index]);
        contain_flags[quantum_index] = nullptr;
    }
    excluded[quantum_index] = false;
}

void memory_cache::get_cache_miss_cnt_statistics(int key, int number_of_elements) {
#if (ENABLE_STATISTICS_COLLECTION)
  #if (ENABLE_STATISTICS_EVERY_CACHE_MISSES)
    if (statistic_file_stream.is_open()) {
        statistic_file_stream.close();
    }
  #endif
  #if (ENABLE_STATISTICS_CACHE_MISSES_CNT)
    std::vector<int> cache_miss_cnts(size-1);
    MPI_Gather(&cache_miss_cnt, 1, MPI_INT, cache_miss_cnts.data(), 1, MPI_INT, 0, workers_comm);
    if (rank == 1) {
        cache_miss_cnt_file_stream.open("cache_miss_cnt.txt", std::ios_base::app);
        cache_miss_cnt_file_stream << "------------------------------\n";
        cache_miss_cnt_file_stream << "cache_size: " << cache_memory.size() << "; number_of_elements: " << number_of_elements << "; number_of_processes: " << size << "; key: " << key <<";\n";
        int cnt = 0;
        for (int i = 0; i < (int)cache_miss_cnts.size(); ++i) {
            cache_miss_cnt_file_stream << "rank " << i + 1 << ": "<< cache_miss_cnts[i] << "; ";
            cnt += cache_miss_cnts[i];
        }
        cache_miss_cnt_file_stream << "\n";
        cache_miss_cnt_file_stream << "total cache misses: " << cnt <<"; total сache evictions: " << cache_miss_cnt_no_free << "\n";
        cache_miss_cnt_file_stream << "------------------------------\n";
        cache_miss_cnt_file_stream.close();
    }
  #endif
#endif

}
