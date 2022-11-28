#include "memory_manager.h"

// посылка мастеру: [операция; идентификатор структуры, откуда требуются данные; требуемый номер кванта]
// посылка рабочему от мастера: [операция; идентификатор структуры, откуда требуются данные;
//                               требуемый номер кванта; номер процесса, которому требуется передать квант]
// если номер структуры = -1, то завершение функций worker_helper_thread и master_helper_thread


std::vector<memory_line_common*> memory_manager::memory;  // структура-хранилище памяти и вспомогательной информации
std::thread memory_manager::helper_thr;  // вспомогательный поток
int memory_manager::rank;  // ранг процесса в MPI
int memory_manager::size;  // число процессов в MPI
int memory_manager::worker_rank;  // worker_rank = rank-1
int memory_manager::worker_size;  // worker_size = size-1
int memory_manager::proc_count_ready = 0;
MPI_File memory_manager::fh;
MPI_Comm memory_manager::workers_comm;
schedule memory_manager::sch;

StatusCode memory_manager::init(int argc, char** argv, std::string error_helper_str, bool is_statistic, config* cfg) {
    int provided = 0;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided != MPI_THREAD_MULTIPLE) {
        std::cout<<"MPI_THREAD_MULTIPLE is not supported!" << std::endl;
        exit(1);
    }
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (size < 2) {
        std::cout << "Error: you need at least 2 processes!" << std::endl;
        if (error_helper_str != "")
            std::cout <<"Usage:\n" << error_helper_str << std::endl;
        MPI_Finalize();
        exit(1);
    }
    worker_rank = rank - 1;
    worker_size = size - 1;
    if (rank == 0) {
        helper_thr = std::thread(master_helper_thread);
    } else {
        helper_thr = std::thread(worker_helper_thread);
    }

    std::vector<int> procs(size-1);
    for (int i = 1; i < size; ++i)
        procs[i-1] = i;
    MPI_Group group_world, group_workers;
    MPI_Comm_group(MPI_COMM_WORLD, &group_world);
    MPI_Group_incl(group_world, worker_size, procs.data(), &group_workers);
    MPI_Comm_create(MPI_COMM_WORLD, group_workers, &workers_comm);
    // TODO: создание своего типа для пересылки посылок ???

    if (is_statistic) {
        StatusCode sts = readStatistic(cfg);
        if (sts != StatusCode::STATUS_OK) {
            return sts;
        }
    }
    return StatusCode::STATUS_OK;
}

int memory_manager::get_MPI_rank() {
    return rank;
}

int memory_manager::get_MPI_size() {
    return size;
}

int memory_manager::get_quantum_index(int key, int index) {
    return index/memory_manager::memory[key]->quantum_size;
}

int memory_manager::get_quantum_size(int key) {
    return memory_manager::memory[key]->quantum_size;
}

void worker_helper_thread() {
    int request[4] = {-2, -2, -2, -2};
    MPI_Status status;
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    while (true) {
        MPI_Recv(request, 4, MPI_INT, MPI_ANY_SOURCE, SEND_DATA_TO_HELPER, MPI_COMM_WORLD, &status);
        if (request[0] == -1 && request[1] == -1 && request[2] == -1 && request[3] == -1) {  // окончание работы вспомогательного потока
            // освобождение памяти
#if (ENABLE_STATISTICS_COLLECTION)
    #if (ENABLE_STATISTICS_QUANTUMS_CNT_WORKERS)
            // write quantum request statistic to file
            std::ofstream worker_process_statistic;
            worker_process_statistic.open(STATISTICS_OUTPUT_DIRECTORY + "quantums_ranks_cnt_process_" + std::to_string(rank) + ".txt");
            worker_process_statistic << "key | quantum_index | cnt | mode (READ_ONLY = 0, READ_WRITE = 1)\n";
    #endif
#endif
            for (int key = 0; key < int(memory_manager::memory.size()); ++key) {
#if (ENABLE_STATISTICS_COLLECTION)
    #if (ENABLE_STATISTICS_CACHE_MISSES_CNT)
                auto* memory = dynamic_cast<memory_line_worker*>(memory_manager::memory[key]);
                memory->cache.get_cache_miss_cnt_statistics(key, memory->quantums.size() * memory->quantum_size);
    #endif
    #if (ENABLE_STATISTICS_QUANTUMS_CNT_WORKERS)
                for (int quantum_index = 0; quantum_index < memory->quantums.size(); ++quantum_index) {
                    for (int j = 0; j < memory->quantums[quantum_index].cnt.size(); ++j) {
                        worker_process_statistic << key << " " << quantum_index << " " << memory->quantums[quantum_index].cnt[j] << " " << memory->quantums[quantum_index].modes[j] << "\n";
                    }
                }
    #endif
#endif
                auto* memory_line = dynamic_cast<memory_line_worker*>(memory_manager::memory[key]);
                delete memory_line;
            }
#if (ENABLE_STATISTICS_COLLECTION)
    #if (ENABLE_STATISTICS_QUANTUMS_CNT_WORKERS)
            worker_process_statistic.close();
    #endif
#endif
            break;
        }
        int key = request[1], quantum_index = request[2], to_rank = request[3];
        auto* memory = dynamic_cast<memory_line_worker*>(memory_manager::memory[key]);
        CHECK(key >= 0 && key < (int)memory_manager::memory.size(), STATUS_ERR_OUT_OF_BOUNDS);
        if (request[0] != PRINT) {
            CHECK(quantum_index >= 0 && quantum_index < (int)memory->quantums.size(), STATUS_ERR_OUT_OF_BOUNDS);
            CHECK(memory->quantums[quantum_index].quantum != nullptr, STATUS_ERR_NULLPTR);
            if (request[0] != DELETE)
                CHECK(to_rank > 0 && to_rank < size, STATUS_ERR_WRONG_RANK);
        }
        // запросы на GET_DATA_R и GET_DATA_RW принимаются только от мастера
        switch(request[0]) {
            case GET_DATA_R:  // READ_ONLY режим, запись запрещена, блокировка мьютекса для данного кванта не нужна
                MPI_Send(memory->quantums[quantum_index].quantum, memory->quantum_size,
                                        memory->type, to_rank, GET_DATA_FROM_HELPER, MPI_COMM_WORLD);
                break;
            case GET_DATA_RW:  // READ_WRITE режим
                memory->quantums[quantum_index].mutex->lock();
                MPI_Send(memory->quantums[quantum_index].quantum, memory->quantum_size,
                                        memory->type, to_rank, GET_DATA_FROM_HELPER, MPI_COMM_WORLD);
                memory->allocator.free(reinterpret_cast<char**>(&(memory->quantums[quantum_index].quantum)));  // после отправки данных в READ_WRITE режиме квант на данном процессе удаляется
                memory->quantums[quantum_index].mutex->unlock();
                break;
            case PRINT:
            {
                int l_quantum_index = request[2], r_quantum_index = request[3];
                for (int i = l_quantum_index; i < r_quantum_index; ++i)
                    memory_manager::print_quantum(key, i);
                int ready = 1;
                MPI_Send(&ready, 1, MPI_INT, 0, GET_PERMISSION_TO_CONTINUE, MPI_COMM_WORLD);
                break;
            }
            case DELETE:
            {
                memory->quantums[quantum_index].mutex->lock();
                int removing_quantum_index = request[2];
                memory->allocator.free(reinterpret_cast<char**>(&(memory->quantums[removing_quantum_index].quantum)));
                memory->quantums[quantum_index].mutex->unlock();
                break;
            }
        }
    }
}

void master_helper_thread() {
    int request[4] = {-2, -2, -2, -2};
    MPI_Status status;
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

#if (ENABLE_STATISTICS_COLLECTION)
  #if (ENABLE_STATISTICS_QUANTUMS_SCHEDULE)
    std::ofstream quantums_schedule_file_stream;
    quantums_schedule_file_stream.open(STATISTICS_OUTPUT_DIRECTORY + "quantums_schedule_raw" + ".txt");
  #endif
#endif
    while (true) {
        MPI_Recv(&request, 4, MPI_INT, MPI_ANY_SOURCE, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD, &status);
        if (request[0] == -1 && request[1] == -1 && request[2] == -1) {  // окончание работы вспомогательного потока
            for (auto _line: memory_manager::memory) {
                memory_line_master* line = dynamic_cast<memory_line_master*>(_line);
                for (auto quantum: line->quantums) {
                    for (int i = 0; i < size - 1; ++i) {
                        CHECK(quantum.requests[i] == 0, STATUS_ERR_UNKNOWN);
                    }
                }
                delete _line;
            }
            break;
        }
        int key = request[1], quantum_index = request[2];
        memory_line_master* memory;
        memory = dynamic_cast<memory_line_master*>(memory_manager::memory[key]);
        CHECK(key >= 0 && key < (int)memory_manager::memory.size(), STATUS_ERR_OUT_OF_BOUNDS);
        if (request[0] != PRINT) {
            CHECK(quantum_index >= 0 && quantum_index < (int)memory->quantums.size(), STATUS_ERR_OUT_OF_BOUNDS);
        }
        switch(request[0]) {
            case LOCK:  // блокировка кванта
                if (memory->quantums[quantum_index].quantum_lock_number == -1) {  // квант не заблокирован
                    int to_rank = status.MPI_SOURCE;
                    int tmp = 1;
                    memory->quantums[quantum_index].quantum_lock_number = status.MPI_SOURCE;
                    MPI_Send(&tmp, 1, MPI_INT, to_rank, GET_DATA_FROM_MASTER_HELPER_LOCK, MPI_COMM_WORLD);  // уведомление о том,
                                                                                                            // что процесс может заблокировать квант
                } else {  // квант уже заблокирован другим процессом, данный процесс помещается в очередь ожидания по данному кванту
                    memory->wait_locks.push(quantum_index, status.MPI_SOURCE);
                }
                break;
            case UNLOCK:  // разблокировка кванта
                if (memory->quantums[quantum_index].quantum_lock_number == status.MPI_SOURCE) {
                    memory->quantums[quantum_index].quantum_lock_number = -1;
                    if (memory->wait_locks.is_contain(quantum_index)) {  // проверка, есть ли в очереди ожидания по данному кванту какой-либо процесс
                        int to_rank = memory->wait_locks.pop(quantum_index);
                        memory->quantums[quantum_index].quantum_lock_number = to_rank;
                        int tmp = 1;
                        MPI_Send(&tmp, 1, MPI_INT, to_rank, GET_DATA_FROM_MASTER_HELPER_LOCK, MPI_COMM_WORLD);  // уведомление о том, что процесс, изъятый
                                                                                                                // из очереди, может заблокировать квант
                    }
                }
                break;
            case GET_INFO:  // получить квант
                if (memory->quantums[quantum_index].mode == READ_ONLY) {
                    if (memory->quantums[quantum_index].is_mode_changed) {  // был переход между режимами?
                        CHECK(!memory->quantums[quantum_index].owners.empty(), STATUS_ERR_READ_UNINITIALIZED_DATA);  // квант не был инициализирован
                        CHECK(memory->quantums[quantum_index].quantum_ready == true, STATUS_ERR_UNKNOWN);
                        CHECK(memory->quantums[quantum_index].owners.size() == 1, STATUS_ERR_UNKNOWN);
                        memory->quantums[quantum_index].is_mode_changed = false;
                        int to_rank = memory->quantums[quantum_index].owners.front();
                        if (to_rank == status.MPI_SOURCE) {  // после перехода оказалось, что квант находится на процессе, который отправил запрос?
                            // do smth with cache?
                            MPI_Send(&to_rank, 1, MPI_INT, status.MPI_SOURCE, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD);
                            break;
                        }
                    }
                    CHECK(!memory->quantums[quantum_index].owners.empty(), STATUS_ERR_READ_UNINITIALIZED_DATA);  // квант не был инициализирован

                    int to_rank = memory_manager::get_owner(key, quantum_index, status.MPI_SOURCE);  // получение ранга наиболее предпочтительного процесса
                    CHECK(to_rank > 0 && to_rank < size, STATUS_ERR_WRONG_RANK);

                    int to_request[4] = {GET_DATA_R, key, quantum_index, status.MPI_SOURCE};
                    MPI_Send(&to_rank, 1, MPI_INT, status.MPI_SOURCE, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD);  // отправление информации о том, с каким процессом
                                                                                                                     // нужно взаимодействовать для получения кванта
                    if (to_rank != status.MPI_SOURCE) {
                        CHECK(to_rank < size, STATUS_ERR_OUT_OF_BOUNDS);
                        ++memory->quantums[quantum_index].requests[to_rank - 1];
                        MPI_Send(to_request, 4, MPI_INT, to_rank, SEND_DATA_TO_HELPER, MPI_COMM_WORLD);  // отправление запроса вспомогательному потоку
                                                                                                         // процесса-рабочего о переслыке данных
                    }

                    // работа с кешем
                    int removing_quantum_index = request[3];
                    if (removing_quantum_index >= 0) {
                        memory_manager::remove_owner(key, removing_quantum_index, status.MPI_SOURCE);
                        // нет необработанных запросов на передачу данного кванта с данного процесса?
                        if (memory->quantums[removing_quantum_index].requests[status.MPI_SOURCE] == 0) {
                            int request_to_delete[] = {DELETE, key, removing_quantum_index, -1};
                            MPI_Send(request_to_delete, 4, MPI_INT, status.MPI_SOURCE, SEND_DATA_TO_HELPER, MPI_COMM_WORLD);
                        } else {
                            memory->quantums[removing_quantum_index].want_to_delete[status.MPI_SOURCE] = true;
                        }
                    }

                } else {  // READ_WRITE mode
                    if (memory->quantums[quantum_index].is_mode_changed) {  // был переход между режимами?
                        int to_rank = memory_manager::get_owner(key, quantum_index, status.MPI_SOURCE);  // получение ранга наиболее предпочтительного процесса
                        memory->quantums[quantum_index].is_mode_changed = false;
                        while (memory->quantums[quantum_index].owners.size())
                            memory->quantums[quantum_index].owners.pop_front();  // TODO: send to all quantums to free memory?
                        memory->quantums[quantum_index].quantum_ready = false;
                        memory->quantums[quantum_index].owners.push_back(status.MPI_SOURCE);
                        if (to_rank == -1 || to_rank == status.MPI_SOURCE) {  // данные у процесса, отправившего запрос?
                            MPI_Send(&status.MPI_SOURCE, 1, MPI_INT, status.MPI_SOURCE, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD);
                        } else {
                            CHECK(to_rank < size, STATUS_ERR_OUT_OF_BOUNDS);
                            ++memory->quantums[quantum_index].requests[to_rank - 1];
                            int to_request[4] = {GET_DATA_RW, key, quantum_index, status.MPI_SOURCE};
                            MPI_Send(&to_rank, 1, MPI_INT, status.MPI_SOURCE, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD);  // отправление информации о том, с каким процессом
                                                                                                                             // нужно взаимодействовать для получения кванта
                            MPI_Send(to_request, 4, MPI_INT, to_rank, SEND_DATA_TO_HELPER, MPI_COMM_WORLD);  // отправление запроса вспомогательному потоку
                                                                                                             // процесса-рабочего о переслыке данных
                        }
                        break;
                    }

                    if (memory->quantums[quantum_index].owners.empty()) {  // данные ранее не запрашивались?
                        memory->quantums[quantum_index].quantum_ready = false;
                        memory->quantums[quantum_index].owners.push_back(status.MPI_SOURCE);
                        MPI_Send(&status.MPI_SOURCE, 1, MPI_INT, status.MPI_SOURCE, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD);  // отправление информации о том, что процесс,
                                                                                                                                   // отправивший запрос, может забрать квант без
                                                                                                                                   // пересылок данных
                        break;
                    }  // empty

                    if (memory->quantums[quantum_index].quantum_ready) {  // данные готовы к пересылке?
                        CHECK(memory->quantums[quantum_index].owners.size() == 1, STATUS_ERR_UNKNOWN);
                        int to_rank = memory->quantums[quantum_index].owners.front();
                        int to_request[4] = {GET_DATA_RW, key, quantum_index, status.MPI_SOURCE};
                        memory->quantums[quantum_index].quantum_ready = false;
                        memory->quantums[quantum_index].owners.pop_front();
                        memory->quantums[quantum_index].owners.push_back(status.MPI_SOURCE);

                        CHECK(to_rank < size, STATUS_ERR_OUT_OF_BOUNDS);
                        ++memory->quantums[quantum_index].requests[to_rank - 1];
                        MPI_Send(&to_rank, 1, MPI_INT, status.MPI_SOURCE, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD);  // отправление информации о том, с каким процессом
                                                                                                                         // нужно взаимодействовать для получения кванта
                        MPI_Send(to_request, 4, MPI_INT, to_rank, SEND_DATA_TO_HELPER, MPI_COMM_WORLD);  // отправление запроса вспомогательному потоку
                                                                                                         // процесса-рабочего о переслыке данных
                    } else {  // данные не готовы к пересылке (в данный момент пересылаются другому процессу)
                        memory->wait_quantums.push(quantum_index, status.MPI_SOURCE);
                    }
                }
                break;
            case SET_INFO:  // данные готовы для пересылки
            {
                int worker_rank_sender = request[3] - 1;
                if (worker_rank_sender >= 0) {
                    CHECK(worker_rank_sender < size - 1, STATUS_ERR_OUT_OF_BOUNDS);
                    // уменьшить счётчик для кванта и процесса, посылавшего квант на процесс status.MPI_SOURCE
                    --memory->quantums[quantum_index].requests[worker_rank_sender];
                    CHECK(memory->quantums[quantum_index].requests[worker_rank_sender] >= 0, STATUS_ERR_UNKNOWN);
                    // для данного процесса и кванта незаконченных запросов не осталось?
                    if (memory->quantums[quantum_index].requests[worker_rank_sender] == 0 &&
                                memory->quantums[quantum_index].want_to_delete[worker_rank_sender]) {

                        memory->quantums[quantum_index].want_to_delete[worker_rank_sender] = false;
                        int request_to_delete[] = {DELETE, key, quantum_index, 0};
                        MPI_Send(request_to_delete, 4, MPI_INT, worker_rank_sender, SEND_DATA_TO_HELPER, MPI_COMM_WORLD);
                    }
                }

                if (memory->quantums[quantum_index].mode == READ_ONLY) {
                    memory->quantums[quantum_index].owners.push_back(status.MPI_SOURCE); // процесс помещается в вектор процессов,
                                                                                 // которые могут пересылать данный квант другим процессам
                } else {  // READ_WRITE mode
                    CHECK(memory->quantums[quantum_index].owners.front() == status.MPI_SOURCE, STATUS_ERR_UNKNOWN);
                    CHECK(memory->quantums[quantum_index].quantum_ready == false, STATUS_ERR_UNKNOWN);
                    memory->quantums[quantum_index].quantum_ready = true;
                    if (memory->wait_quantums.is_contain(quantum_index)) {  // есть процессы, ожидающие готовности кванта?
                        int source_rank = memory->wait_quantums.pop(quantum_index);
                        int to_rank = memory->quantums[quantum_index].owners.front();
                        memory->quantums[quantum_index].quantum_ready = false;
                        CHECK(memory->quantums[quantum_index].owners.size() == 1, STATUS_ERR_UNKNOWN);
                        memory->quantums[quantum_index].owners.pop_front();
                        memory->quantums[quantum_index].owners.push_back(source_rank);
                        int to_request[4] = {GET_DATA_RW, key, quantum_index, source_rank};
                        CHECK(source_rank != to_rank, STATUS_ERR_WRONG_RANK);
                        CHECK(to_rank > 0 && to_rank < size, STATUS_ERR_WRONG_RANK);

                        CHECK(to_rank < size, STATUS_ERR_OUT_OF_BOUNDS);
                        ++memory->quantums[quantum_index].requests[to_rank - 1];
                        MPI_Send(&to_rank, 1, MPI_INT, source_rank, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD);  // отправление информации о том, с каким процессом
                                                                                                                   // нужно взаимодействовать для получения кванта
                        MPI_Send(to_request, 4, MPI_INT, to_rank, SEND_DATA_TO_HELPER, MPI_COMM_WORLD);  // отправление запроса вспомогательному потоку
                                                                                                         // процесса-рабочего о переслыке данных
                    }
                }
#if (ENABLE_STATISTICS_COLLECTION)
  #if (ENABLE_STATISTICS_QUANTUMS_SCHEDULE)
                std::string info = std::to_string(key) + " " + std::to_string(quantum_index) + " " + std::to_string(status.MPI_SOURCE);
                quantums_schedule_file_stream << info << "\n";
  #endif
#endif
                break;
            }
            case CHANGE_MODE:  // изменить режим работы с памятью
            {
                int quantum_l = request[2], quantum_r = request[3];
                ++memory->quantums[quantum_l].num_of_changed_mode_procs;
                if (memory->quantums[quantum_l].num_of_changed_mode_procs == memory_manager::worker_size) {  // все процессы дошли до этапа изменения режима?
                    int ready = 1;
                    for (int i = 1; i < size; ++i) {
                        MPI_Send(&ready, 1, MPI_INT, i, GET_PERMISSION_FOR_CHANGE_MODE, MPI_COMM_WORLD);  // информирование о смене режима и о том, что
                                                                                                          // другие процессы могут продолжить выполнение программы дальше
                    }
                    memory->quantums[quantum_l].num_of_changed_mode_procs = 0;
                    for (int i = quantum_l; i < quantum_r; ++i) {
                        memory->quantums[i].is_mode_changed = true;
                        if (memory->quantums[i].mode == READ_ONLY) {
                            memory->quantums[i].mode = READ_WRITE;
                        } else {
                            memory->quantums[i].mode = READ_ONLY;
                        }
                    }
#if (ENABLE_STATISTICS_COLLECTION)
  #if (ENABLE_STATISTICS_QUANTUMS_SCHEDULE)
                std::string info = std::to_string(key) + " " + "CHANGE_MODE " + std::to_string(quantum_l) + " " + std::to_string(quantum_r);
                quantums_schedule_file_stream << info << "\n";
  #endif
#endif
                }
                break;
            }
            case PRINT:
            {
                ++memory_manager::proc_count_ready;
                if (memory_manager::proc_count_ready == memory_manager::worker_size) {
                    int l_quantum_index = 0, r_quantum_index = 0;
                    memory_manager::proc_count_ready = 0;
                    std::set<int>s1, s2;
                    while (l_quantum_index < (int)memory->quantums.size()) {
                        if (r_quantum_index < (int)memory->quantums.size()) {
                            CHECK(memory->quantums[r_quantum_index].quantum_ready, STATUS_ERR_NULLPTR);
                        }
                        if (s1.empty()) {
                            for (auto proc: memory->quantums[r_quantum_index].owners)
                                s1.insert(proc);
                            ++r_quantum_index;
                            continue;
                        } else {
                            if (r_quantum_index < (int)memory->quantums.size())
                                for (auto proc: memory->quantums[r_quantum_index].owners)
                                    if (s1.find(proc) != s1.end())
                                        s2.insert(proc);
                            if (s2.size() > 0) {
                                s1 = s2;
                                s2.clear();
                                ++r_quantum_index;
                            } else {
                                int request[4] = {PRINT, key, l_quantum_index, r_quantum_index};  // [l, r)
                                int to_rank = *s1.begin();
                                MPI_Send(request, 4, MPI_INT, to_rank, SEND_DATA_TO_HELPER, MPI_COMM_WORLD);
                                s1.clear();
                                l_quantum_index = r_quantum_index;
                                int ready;
                                MPI_Status status;
                                MPI_Recv(&ready, 1, MPI_INT, to_rank, GET_PERMISSION_TO_CONTINUE, MPI_COMM_WORLD, &status);  // TODO: IRecv
                            }
                        }
                    }
                    int ready = 1;
                    for (int i = 1; i < size; ++i) {
                        MPI_Send(&ready, 1, MPI_INT, i, GET_PERMISSION_TO_CONTINUE, MPI_COMM_WORLD);
                    }
                }
                break;
            }
        }
    }
#if (ENABLE_STATISTICS_COLLECTION)
  #if (ENABLE_STATISTICS_QUANTUMS_SCHEDULE)
    if (quantums_schedule_file_stream.is_open()) {
        quantums_schedule_file_stream.close();
    }
  #endif
#endif
}

void memory_manager::set_lock(int key, int quantum_index) {
    int request[] = {LOCK, key, quantum_index};
    MPI_Send(request, 3, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);  // отправление мастеру запроса о блокировке кванта
    int ans;
    MPI_Status status;
    MPI_Recv(&ans, 1, MPI_INT, 0, GET_DATA_FROM_MASTER_HELPER_LOCK, MPI_COMM_WORLD, &status);  // квант заблокирован
}

void memory_manager::unset_lock(int key, int quantum_index) {
    int request[3] = {UNLOCK, key, quantum_index};
    MPI_Send(request, 3, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);  // отправление мастеру запроса о разблокировке кванта
}

void memory_manager::change_mode(int key, int quantum_index_l, int quantum_index_r, mods mode) {  // block quantums [l, r)
    // информирование мастера о том, что данный процесс дошёл до этапа изменения режима работы с памятью
    int request[4] = {CHANGE_MODE, key, quantum_index_l, quantum_index_r};
    MPI_Send(request, 4, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);
    int is_ready;
    MPI_Status status;
    MPI_Recv(&is_ready, 1, MPI_INT, 0, GET_PERMISSION_FOR_CHANGE_MODE, MPI_COMM_WORLD, &status);  // после получения ответа данный процесс может продолжить выполнение
    auto* memory = dynamic_cast<memory_line_worker*>(memory_manager::memory[key]);
    for (int i = quantum_index_l; i < quantum_index_r; ++i) {

        // работа с кешем
        memory->quantums[i].mutex->lock();
        if (mode == READ_ONLY) {
            if (memory->quantums[i].quantum != nullptr) {
                memory->cache.add_to_excluded(i);
            }
        } else {  // READ_WRITE
            if (memory->quantums[i].quantum != nullptr &&
                    (memory->cache.is_contain(i) || memory->cache.is_excluded(i))) {
                memory->cache.delete_elem(i);
            }
        }
        memory->quantums[i].mutex->unlock();
        memory->quantums[i].is_mode_changed = true;
        memory->quantums[i].mode = mode;
    }
}

void memory_manager::print(int key, const std::string& path) {
    int err = MPI_File_open(workers_comm, path.data(), MPI_MODE_WRONLY | MPI_MODE_CREATE | MPI_MODE_APPEND, MPI_INFO_NULL, &fh);\
    CHECK(!err, STATUS_ERR_FILE_OPEN);
    int request[4] = {PRINT, key, -1, -1};
    MPI_Send(request, 4, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);
    int is_ready;
    MPI_Status status;
    MPI_Recv(&is_ready, 1, MPI_INT, 0, GET_PERMISSION_TO_CONTINUE, MPI_COMM_WORLD, &status);
    MPI_File_close(&fh);
}

void memory_manager::print_quantum(int key, int quantum_index) {
    CHECK(memory_manager::rank >= 1 && memory_manager::rank < size, STATUS_ERR_WRONG_RANK);
    auto* memory = dynamic_cast<memory_line_worker*>(memory_manager::memory[key]);
    CHECK(memory->quantums[quantum_index].quantum != nullptr, STATUS_ERR_NULLPTR);
    MPI_Status status;
    MPI_File_write_at(fh, quantum_index * memory->quantum_size * memory->size_of, memory->quantums[quantum_index].quantum, std::min(memory->logical_size, memory->quantum_size), memory->type, &status);
    std::cout<<std::flush;
}

MPI_Datatype memory_manager::get_MPI_datatype(int key) {
    return dynamic_cast<memory_line_worker*>(memory_manager::memory[key])->type;
}

void memory_manager::wait_all() {
    MPI_Barrier(MPI_COMM_WORLD);
}

void memory_manager::wait_all_workers() {
    if (rank != 0)
        MPI_Barrier(workers_comm);
}

int memory_manager::wait() {
    char x;
    MPI_Status status;
    MPI_Recv(&x, 1, MPI_CHAR, MPI_ANY_SOURCE, NOTIFY, MPI_COMM_WORLD, &status);
    return status.MPI_SOURCE;
}

void memory_manager::wait(int from_rank) {
    char x;
    MPI_Status status;
    MPI_Recv(&x, 1, MPI_CHAR, from_rank, NOTIFY, MPI_COMM_WORLD, &status);
}

void memory_manager::notify(int to_rank) {
    char x = 42;
    MPI_Send(&x, 1, MPI_CHAR, to_rank, NOTIFY, MPI_COMM_WORLD);
}

void memory_manager::finalize() {
    if (rank != 0) {
        int tmp = 1;
        MPI_Send(&tmp, 1, MPI_INT, 0, FINALIZE_WORKER, MPI_COMM_WORLD);
        MPI_Status status;
        MPI_Recv(&tmp, 1, MPI_INT, 0, FINALIZE_MASTER, MPI_COMM_WORLD, &status);
    } else {
        int tmp;
        for (int i = 1; i < size; ++i) {
            MPI_Status status;
            MPI_Recv(&tmp, 1, MPI_INT, i, FINALIZE_WORKER, MPI_COMM_WORLD, &status);
        }
        for (int i = 1; i < size; ++i) {
            MPI_Send(&tmp, 1, MPI_INT, i, FINALIZE_MASTER, MPI_COMM_WORLD);
        }
    }
    if (rank == 0) {
        int request[4] = {-1, -1, -1, -1};
        for (int i = 1; i < size; ++i) {
            MPI_Send(request, 4, MPI_INT, i, SEND_DATA_TO_HELPER, MPI_COMM_WORLD);  // завершение работы вспомогательных потоков процессов рабочих
        }
    } else if (rank == 1) {
        int request[4] = {-1, -1, -1, -1};
        MPI_Send(request, 3, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);  // завершение работы вспомогательного потока процесса-мастера
    }
    CHECK(helper_thr.joinable(), STATUS_ERR_UNKNOWN);
    helper_thr.join();
    // if (rank != 0) {
    //     for (int key = 0; key < (int)memory.size(); ++key) {
    //         auto* memory = dynamic_cast<memory_line_worker*>(memory_manager::memory[key]);
    //         assert(memory[key].wait_quantums.size() == 0);
    //         if (!memory_manager::is_read_only_mode) {
    //             for (int quantum = 0; quantum < (int)memory[key].quantum_owner.size(); ++quantum) {
    //                 assert(memory[key].quantum_owner[quantum].first == true);
    //             }
    //         }
    //         for (int i = 0; i < (int)memory->quantums.size(); ++i) {
    //             assert(memory[key].quantums[i] == nullptr);
    //         }
    //     }
    // }
    MPI_Finalize();
}

int memory_manager::get_owner(int key, int quantum_index, int requesting_process) {
    auto* memory = dynamic_cast<memory_line_master*>(memory_manager::memory[key]);
    CHECK(!memory->quantums[quantum_index].owners.empty(), STATUS_ERR_READ_UNINITIALIZED_DATA);
    for (auto rank: memory->quantums[quantum_index].owners)
        if (rank == requesting_process)
            return requesting_process;
    int to_rank = memory->quantums[quantum_index].owners.front();
    memory->quantums[quantum_index].owners.pop_front();
    memory->quantums[quantum_index].owners.push_back(to_rank);
    return to_rank;
}

void memory_manager::remove_owner(int key, int removing_quantum_index, int process) {
    auto* memory = dynamic_cast<memory_line_master*>(memory_manager::memory[key]);
    CHECK(!memory->quantums[removing_quantum_index].owners.empty(), STATUS_ERR_UNKNOWN);
    for (auto rank = memory->quantums[removing_quantum_index].owners.begin();
                        rank  < memory->quantums[removing_quantum_index].owners.end(); ++rank) {
        if (*rank == process) {
            memory->quantums[removing_quantum_index].owners.erase(rank);
            break;
        }
    }
}

StatusCode memory_manager::readStatistic(config* cfg) {
    if (rank == 0) {
        if (cfg->is_schedule_statistic) {
            StatusCode sts = memory_manager::sch.read_from_file_schedule(cfg->schedule_statistic_file_path);
            if (sts != StatusCode::STATUS_OK) {
                return sts;
            }
        }
    }
    else {
        if (cfg->is_quantums_access_cnt_statistic) {
            if (rank - 1 < cfg->quantums_access_cnt_statistic_file_path.size()) {
                StatusCode sts = memory_manager::sch.read_from_file_schedule(cfg->schedule_statistic_file_path);
                if (sts != StatusCode::STATUS_OK) {
                    return sts;
                }
            }
        }
    }
    return StatusCode::STATUS_OK;
}
