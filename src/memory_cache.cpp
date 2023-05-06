#include "memory_cache.h"
#include "common.h"

#if (ENABLE_STATISTICS_COLLECTION)
std::ofstream memory_cache::statistic_file_stream;
#endif

void memory_cache::init(int cache_size, int number_of_quantums, MPI_Comm comm, int key,
        const std::string& statistics_output_directory, statistic* stat) {
    cache_memory = std::vector<cache_node>(cache_size, {-1, nullptr, nullptr});
    contain_flags = std::vector<cache_node*>(number_of_quantums, nullptr);
    excluded = std::vector<bool>(number_of_quantums, false);
    workers_comm = comm;
    this->key = key;
    this->stat = stat;
    for (int i = 0; i < cache_size; ++i) {
        free_cache_nodes.push_back(&cache_memory[i]);
    }
    if (stat) {
        vector_quantum_cluster_info = stat->get_vectors_quantums_clusters();
        if (vector_quantum_cluster_info->size()) {
            CHECK(key < vector_quantum_cluster_info->size(), STATUS_ERR_UNKNOWN);
        }
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    this->key = key;

    this->statistics_output_directory = statistics_output_directory;
#if (ENABLE_STATISTICS_COLLECTION)
  #if (ENABLE_STATISTICS_EVERY_CACHE_MISSES)
    std::string path = this->statistics_output_directory + "memory_cache_" + std::to_string(rank) + ".txt";
    if (!statistic_file_stream.is_open()) {
        statistic_file_stream.open(this->statistics_output_directory + "memory_cache_" + std::to_string(rank) + ".txt");
    }
  #endif
#endif
}

int memory_cache::add(int quantum_index) {
    CHECK(quantum_index >= 0 && quantum_index < (int)contain_flags.size(), STATUS_ERR_OUT_OF_BOUNDS);
    // элемент уже находится в кеше?
    if (is_contain(quantum_index)) {
        update(quantum_index);
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
    std::string output_str = std::to_string(PUT_IN_CACHE) + " " + std::to_string(key) + " " + std::to_string(quantum_index) + "\n";
    statistic_file_stream << output_str; //" " << MPI_Wtime();
        // PRINT_TO_FILE(statistics_output_directory, "memory_cache", info);
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

    cache_node* node = nullptr;
    // вытеснение кванта из кеша текущим квантом
    if (vector_quantum_cluster_info->size() > 0) {
        int cluster_id_target =  (*vector_quantum_cluster_info)[key][quantum_index].cluster_id;
        for (cache_node* cache_cur_node = cache_indexes.get_begin(); cache_cur_node != nullptr; cache_cur_node = cache_cur_node->next) {
            int cur_id = cache_cur_node->value;
            if ((*vector_quantum_cluster_info)[key][cur_id].cluster_id != cluster_id_target) {
                node = cache_cur_node;
                cache_indexes.delete_node(cache_cur_node);
                break;
            }
        }
    }

    // if there are no statistic info or remove target value after statistic analyzing is still unknown, use LRU algorigthm
    if (node == nullptr) {
        node = cache_indexes.pop_front();
    }
    contain_flags[node->value] = nullptr;
    int return_value = node->value;
#if (ENABLE_STATISTICS_COLLECTION)
  #if (ENABLE_STATISTICS_EVERY_CACHE_MISSES)
    // output_str = std::to_string(REMOVE_FROM_CACHE) + " " + std::to_string(key) + " " + std::to_string(node->value) + "\n";
    // statistic_file_stream << output_str;  // " " << MPI_Wtime();
  #endif
#endif
    node->value = quantum_index;
    contain_flags[quantum_index] = node;
    cache_indexes.push_back(node);
    return return_value;
}

bool memory_cache::is_contain(int quantum_index) {
    CHECK(quantum_index >= 0 && quantum_index < (int)contain_flags.size(), STATUS_ERR_OUT_OF_BOUNDS);
    return contain_flags[quantum_index] != nullptr;
}

void memory_cache::add_to_excluded(int quantum_index) {
    delete_elem(quantum_index);
#if (ENABLE_STATISTICS_COLLECTION)
  #if (ENABLE_STATISTICS_EVERY_CACHE_MISSES)
    std::string output_str = std::to_string(ADD_TO_EXCLUDED) + " " + std::to_string(key) + " " + std::to_string(quantum_index) + " " + std::to_string(MPI_Wtime()) + "\n";
    statistic_file_stream << output_str;
  #endif
#endif
    excluded[quantum_index] = true;

}

bool memory_cache::is_excluded(int quantum_index) {
    CHECK(quantum_index >= 0 && quantum_index < (int)excluded.size(), STATUS_ERR_OUT_OF_BOUNDS);
    return excluded[quantum_index];
}

void memory_cache::delete_elem(int quantum_index) {
    CHECK(quantum_index >= 0 && quantum_index < (int)excluded.size(), STATUS_ERR_OUT_OF_BOUNDS);
    if (is_contain(quantum_index)) {
        cache_indexes.delete_node(contain_flags[quantum_index]);
        free_cache_nodes.push_back(contain_flags[quantum_index]);
        contain_flags[quantum_index] = nullptr;
#if (ENABLE_STATISTICS_COLLECTION)
  #if (ENABLE_STATISTICS_EVERY_CACHE_MISSES)
        std::string output_str = std::to_string(REMOVE_FROM_CACHE) + " " + std::to_string(key) + " " + std::to_string(quantum_index) + " " + std::to_string(MPI_Wtime()) + "\n";
        statistic_file_stream << output_str;
  #endif
#endif
    }
#if (ENABLE_STATISTICS_COLLECTION)
  #if (ENABLE_STATISTICS_EVERY_CACHE_MISSES)
    if (excluded[quantum_index]) {
        std::string output_str = std::to_string(REMOVE_FROM_EXCLUDED) + " " + std::to_string(key) + " " + std::to_string(quantum_index) + " " + std::to_string(MPI_Wtime()) + "\n";
        statistic_file_stream << output_str;
    }
  #endif
#endif
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
    std::vector<int> cache_miss_cnts_no_free(size-1);
    // сбор данных об общем числе кеш-промахов и вытеснений из кеша на 1 процессе
    MPI_Gather(&cache_miss_cnt, 1, MPI_INT, cache_miss_cnts.data(), 1, MPI_INT, 0, workers_comm);
    MPI_Gather(&cache_miss_cnt_no_free, 1, MPI_INT, cache_miss_cnts_no_free.data(), 1, MPI_INT, 0, workers_comm);
    // запись в файл
    if (rank == 1) {
        cache_miss_cnt_file_stream.open(statistics_output_directory + "cache_miss_cnt.txt", std::ios_base::app);
        cache_miss_cnt_file_stream << "------------------------------\n";
        cache_miss_cnt_file_stream << "cache_size: " << cache_memory.size() << "; number_of_elements: " << number_of_elements << "; number_of_processes: " << size << "; key: " << key <<";\n";
        int cnt = 0, cnt_evictions = 0;
        for (int i = 0; i < (int)cache_miss_cnts.size(); ++i) {
            cache_miss_cnt_file_stream << "rank " << i + 1 << ": "<< cache_miss_cnts[i] << "; ";
            cnt += cache_miss_cnts[i];
        }
        cache_miss_cnt_file_stream << "\n";
        for (int i = 0; i < (int)cache_miss_cnts_no_free.size(); ++i) {
            cache_miss_cnt_file_stream << "rank " << i + 1 << ": "<< cache_miss_cnts_no_free[i] << "; ";
            cnt_evictions += cache_miss_cnts_no_free[i];
        }
        cache_miss_cnt_file_stream << "\n";
        cache_miss_cnt_file_stream << "total cache misses: " << cnt <<"; total сache evictions: " << cnt_evictions << "\n";
        cache_miss_cnt_file_stream << "------------------------------\n";
        cache_miss_cnt_file_stream.close();
    }
  #endif
#endif
}

void memory_cache::update(int quantum_index) {
    CHECK(is_contain(quantum_index), STATUS_ERR_UNKNOWN);
#if (ENABLE_STATISTICS_COLLECTION)
  #if (ENABLE_STATISTICS_EVERY_CACHE_MISSES)
    std::string output_str = std::to_string(ALREADY_IN_CACHE) + " " + std::to_string(key) + " " + std::to_string(quantum_index) + "\n";
    statistic_file_stream << output_str;  //" " << MPI_Wtime(); // что-то делать, только если quantum_index не на последнем элементе?
    // PRINT_TO_FILE(statistics_output_directory, "memory_cache", info);
  #endif
#endif
    cache_indexes.delete_node(contain_flags[quantum_index]);
    cache_indexes.push_back(contain_flags[quantum_index]);
}
