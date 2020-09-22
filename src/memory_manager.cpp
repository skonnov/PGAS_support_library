#include <thread>
#include <mpi.h>
#include <vector>
#include <climits>
#include <iostream>
#include <cassert>
#include <mutex>
#include "memory_manager.h"
#include "queue_quantums.h"

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


void memory_manager::memory_manager_init(int argc, char**argv) {
    int provided = 0;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if(provided != MPI_THREAD_MULTIPLE) {
            abort();
    }
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    worker_rank = rank - 1;
    worker_size = size - 1;
    if(rank == 0) {
        helper_thr = std::thread(master_helper_thread);
    } else {
        helper_thr = std::thread(worker_helper_thread);
    }
    // создание своего типа для пересылки посылок ???
}

int memory_manager::get_MPI_rank() {
    return rank;
}

int memory_manager::get_MPI_size() {
    return size;
}

int memory_manager::create_object(int number_of_elements) {
    memory_line_common* line;
    int num_of_quantums = (number_of_elements + QUANTUM_SIZE - 1) / QUANTUM_SIZE;
    if (rank == 0) {
        line = new memory_line_master;
        auto line_master = dynamic_cast<memory_line_master*>(line);
        line_master->quantums_for_lock.resize(num_of_quantums);
        line_master->quantum_owner.resize(num_of_quantums);
        line_master->owners.resize(num_of_quantums);
        line_master->wait_locks.resize(num_of_quantums);
        line_master->wait_quantums.resize(num_of_quantums);
        for (int i = 0; i < int(line_master->quantums_for_lock.size()); i++) {
            line_master->quantums_for_lock[i] = -1;
            line_master->quantum_owner[i] = {0, -1};
            line_master->owners[i] = std::vector<int>();
        }
    } else {
        line = new memory_line_worker;
        auto line_worker = dynamic_cast<memory_line_worker*>(line);
        line_worker->mutexes.resize(num_of_quantums);
        line_worker->quantums.resize(num_of_quantums);
        for (int i = 0; i < num_of_quantums; i++) {
            line_worker->quantums[i] = nullptr;
            line_worker->mutexes[i] = new std::mutex();
        }
    }
    line->mode.resize(num_of_quantums, 0);
    line->num_of_change_mode_procs.resize(num_of_quantums, 0);
    line->logical_size = number_of_elements;
    memory.emplace_back(line);
    MPI_Barrier(MPI_COMM_WORLD);
    return memory.size()-1;
}

int memory_manager::get_data(int key, int index_of_element) {
    assert(key >= 0 && key < (int)memory_manager::memory.size());
    auto* memory = dynamic_cast<memory_line_worker*>(memory_manager::memory[key]);
    int num_quantum = get_quantum_index(index_of_element);
    auto& quantum = memory->quantums[num_quantum];
    if (memory->mode[num_quantum] == READ_WRITE)   // если read_write mode, то используем мьютекс на данный квант
        memory->mutexes[num_quantum]->lock();
    if (quantum != nullptr) {  // на данном процессе есть квант?
        if (!memory->is_mode_changed[num_quantum]) {  // не было изменения режима? (данные актуальны?)
            int elem = quantum[index_of_element%QUANTUM_SIZE];
            if (memory->mode[num_quantum] == READ_WRITE)
                memory->mutexes[num_quantum]->unlock();
            return elem;  // элемент возвращается без обращения к мастеру
        }
    } else {
        quantum = new int[QUANTUM_SIZE];  // выделение памяти
    }
    int request[3] = {GET_INFO, key, num_quantum};  // обращение к мастеру с целью получить квант
    MPI_Send(request, 3, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);
    int to_rank = -2;
    MPI_Status status;
    MPI_Recv(&to_rank, 1, MPI_INT, 0, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD, &status);  // получение ответа от мастера
    // memory->num_change_mode[num_quantum] = memory_manager::num_of_change_mode;
    memory->is_mode_changed[num_quantum] = false; // ???
    if (memory->mode[num_quantum] == READ_ONLY && to_rank == rank) {  // если read_only_mode и данные уже у процесса,
                                                 // ответ мастеру о том, что данные готовы, отправлять не нужно
        return quantum[index_of_element%QUANTUM_SIZE];
    }
    if (to_rank != rank) {  // если данные не у текущего процесса, инициируется передача данных от указанного мастером процесса
        assert(quantum != nullptr);
        assert(to_rank > 0 && to_rank < size);
        MPI_Recv(quantum, QUANTUM_SIZE, MPI_INT, to_rank, GET_DATA_FROM_HELPER, MPI_COMM_WORLD, &status);
    }
    request[0] = SET_INFO;
    MPI_Send(request, 3, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);  // уведомление мастера о том, что данные готовы для передачи другим процессам
    int elem = quantum[index_of_element%QUANTUM_SIZE];
    if (memory->mode[num_quantum] == READ_WRITE)
        memory->mutexes[num_quantum]->unlock();
    return elem;
}

void memory_manager::set_data(int key, int index_of_element, int value) {
    assert(key >= 0 && key < (int)memory_manager::memory.size());
    auto* memory = dynamic_cast<memory_line_worker*>(memory_manager::memory[key]);
    int num_quantum = get_quantum_index(index_of_element);
    if(memory->mode[num_quantum] == READ_WRITE) {
        throw -1;  // запись в READ_ONLY режиме запрещена
    }
    assert(index_of_element >= 0 && index_of_element < (int)memory->logical_size);
    assert(num_quantum >= 0 && num_quantum < (int)memory->quantums.size());
    auto& quantum = memory->quantums[num_quantum];
    assert(num_quantum >= 0 && num_quantum < (int)memory->mutexes.size());
    memory->mutexes[num_quantum]->lock();
    assert((index_of_element%QUANTUM_SIZE) >= 0);
    if (quantum != nullptr) {
        if (!memory->is_mode_changed[key, num_quantum]) {
            quantum[index_of_element%QUANTUM_SIZE] = value;
            memory->mutexes[num_quantum]->unlock();
            return;
        }
    } else {
        quantum = new int[QUANTUM_SIZE];
    }
    int request[3] = {GET_INFO, key, num_quantum};
    MPI_Send(request, 3, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);  // обращение к мастеру с целью получить квант
    int to_rank = -2;
    MPI_Status status;
    MPI_Recv(&to_rank, 1, MPI_INT, 0, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD, &status);  // получение ответа от мастера
    memory->is_mode_changed[num_quantum] = false;
    if (to_rank != rank) {  // если данные не у текущего процесса, инициируется передача данных от указанного мастером процесса
        assert(quantum != nullptr);
        assert(to_rank > 0 && to_rank < size);
        MPI_Recv(quantum, QUANTUM_SIZE, MPI_INT, to_rank, GET_DATA_FROM_HELPER, MPI_COMM_WORLD, &status);
    }
    request[0] = SET_INFO;
    assert(quantum != nullptr);
    quantum[index_of_element%QUANTUM_SIZE] = value;
    MPI_Send(request, 3, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);  // уведомление мастера о том, что данные готовы для передачи другим процессам
    memory->mutexes[num_quantum]->unlock();
}

int memory_manager::get_quantum_index(int index) {
    return index/QUANTUM_SIZE;
}


void worker_helper_thread() {
    int request[4] = {-2, -2, -2, -2};
    MPI_Status status;
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    while(true) {
        MPI_Recv(request, 4, MPI_INT, MPI_ANY_SOURCE, SEND_DATA_TO_HELPER, MPI_COMM_WORLD, &status);
        if(request[0] == -1 && request[1] == -1 && request[2] == -1 && request[3] == -1) {  // окончание работы вспомогательного потока
            // освобождение памяти
            for (int key = 0; key < int(memory_manager::memory.size()); key++) {
                auto* memory_line = dynamic_cast<memory_line_worker*>(memory_manager::memory[key]);
                for(int i = 0; i < int(memory_line->quantums.size()); i++) {
                    if(memory_line->quantums[i] != nullptr) {
                        delete[] memory_line->quantums[i];
                        memory_line->quantums[i] = nullptr;
                    }
                    delete memory_line->mutexes[i];
                }
                delete memory_line;
            }
            break;
        }
        int key = request[1], quantum_index = request[2], to_rank = request[3];
        auto* memory = dynamic_cast<memory_line_worker*>(memory_manager::memory[key]);
        assert(memory->quantums[quantum_index] != nullptr);
        assert(to_rank > 0 && to_rank < size);
        assert(key >= 0 && key < (int)memory_manager::memory.size());
        assert(quantum_index >= 0 && quantum_index < (int)memory->quantums.size());
        // запросы на GET_DATA_R и GET_DATA_RW принимаются только от мастера
        switch(request[0]) {
            case GET_DATA_R:  // READ_ONLY режим, запись запрещена, блокировка мьютекса для данного кванта не нужна
                MPI_Send(memory->quantums[quantum_index], QUANTUM_SIZE,
                                        MPI_INT, to_rank, GET_DATA_FROM_HELPER, MPI_COMM_WORLD);
                break;
            case GET_DATA_RW:  // READ_WRITE режим
                assert(quantum_index >= 0 && quantum_index < (int)memory->mutexes.size());
                memory->mutexes[quantum_index]->lock();
                MPI_Send(memory->quantums[quantum_index], QUANTUM_SIZE,
                                        MPI_INT, to_rank, GET_DATA_FROM_HELPER, MPI_COMM_WORLD);
                delete[] memory->quantums[quantum_index];  // после отправки данных в READ_WRITE режиме квант на данном процессе удаляется
                memory->quantums[quantum_index] = nullptr;
                memory->mutexes[quantum_index]->unlock();
                break;
        }
    }
}

void master_helper_thread() {
    int request[3] = {-2, -2, -2};
    MPI_Status status;
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    while(true) {
        MPI_Recv(&request, 3, MPI_INT, MPI_ANY_SOURCE, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD, &status);
        if (request[0] == -1 && request[1] == -1 && request[2] == -1) {  // окончание работы вспомогательного потока
            for(auto line: memory_manager::memory)
                delete line;
            break;
        }
        int key = request[1], quantum = request[2];
        memory_line_master* memory;
        memory = dynamic_cast<memory_line_master*>(memory_manager::memory[key]);
        assert(key >= 0 && key < (int)memory_manager::memory.size());
        assert(quantum >= 0 && quantum < (int)memory->quantum_owner.size());

        switch(request[0]) {
            case LOCK:  // блокировка кванта
                if (memory->quantums_for_lock[quantum] == -1) {  // квант не заблокирован
                    int to_rank = status.MPI_SOURCE;
                    int tmp = 1;
                    memory->quantums_for_lock[quantum] = status.MPI_SOURCE;
                    MPI_Send(&tmp, 1, MPI_INT, to_rank, GET_DATA_FROM_MASTER_HELPER_LOCK, MPI_COMM_WORLD);  // уведомление о том, 
                                                                                                            // что процесс может заблокировать квант
                } else {  // квант уже заблокирован другим процессом, данный процесс помещается в очередь ожидания по данному кванту
                    memory->wait_locks.push(quantum, status.MPI_SOURCE);
                }
                break;
            case UNLOCK:  // разблокировка кванта
                if (memory->quantums_for_lock[quantum] == status.MPI_SOURCE) {
                    memory->quantums_for_lock[quantum] = -1;
                    if (memory->wait_locks.is_contain(quantum)) {  // проверка, есть ли в очереди ожидания по данному кванту какой-либо процесс
                        int to_rank = memory->wait_locks.pop(quantum);
                        memory->quantums_for_lock[quantum] = to_rank;
                        int tmp = 1;
                        MPI_Send(&tmp, 1, MPI_INT, to_rank, GET_DATA_FROM_MASTER_HELPER_LOCK, MPI_COMM_WORLD);  // уведомление о том, что процесс, изъятый
                                                                                                                // из очереди, может заблокировать квант
                    }
                }
                break;
            case GET_INFO:  // получить квант
                if (memory->mode[quantum] == READ_ONLY) {  // READ_ONLY mode?
                    assert(quantum < (int)memory->quantum_owner.size());
                    assert(quantum < (int)memory->owners.size());
                    if (memory->is_mode_changed[quantum]) {  // был переход между режимами?
                        if (memory->quantum_owner[quantum].second == -1)
                            throw -1;
                        assert(memory->quantum_owner[quantum].first == true);
                        memory->owners[quantum].clear();
                        memory->owners[quantum].push_back(memory->quantum_owner[quantum].second);
                        memory->is_mode_changed[quantum] = false;
                        int to_rank = memory->owners[quantum][0];
                        if (to_rank == status.MPI_SOURCE) {  // после перехода оказалось, что квант находится на процессе, который отправил запрос?
                            MPI_Send(&to_rank, 1, MPI_INT, status.MPI_SOURCE, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD);
                            break;
                        }
                    }
                    if (memory->owners[quantum].empty()) {  // квант не был инициализирован
                            throw -1;
                    }
                    int to_rank = memory_manager::get_owner(key, quantum, status.MPI_SOURCE);  // получение ранга наиболее предпочтительного процесса
                    assert(to_rank > 0 && to_rank < size);
                    int to_request[4] = {GET_DATA_R, key, quantum, status.MPI_SOURCE};
                    MPI_Send(&to_rank, 1, MPI_INT, status.MPI_SOURCE, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD);  // отправление информации о том, с каким процессом
                                                                                                                     // нужно взаимодействовать для получения кванта
                    if(to_rank != status.MPI_SOURCE) {
                        MPI_Send(to_request, 4, MPI_INT, to_rank, SEND_DATA_TO_HELPER, MPI_COMM_WORLD);  // отправление запроса вспомогательному потоку
                                                                                                         // процесса-рабочего о переслыке данных
                    } else {  // в ходе поиска было обнаружено, что
                              // данные уже есть у процесса
                        memory->owners[quantum].push_back(to_rank);
                    }
                } else {  // READ_WRITE mode?
                    if (memory->is_mode_changed[quantum]) {  // был переход между режимами?
                        int to_rank = memory_manager::get_owner(key, quantum, status.MPI_SOURCE);  // получение ранга наиболее предпочтительного процесса
                        memory->is_mode_changed[quantum] = false;
                        memory->quantum_owner[quantum] = {false, status.MPI_SOURCE};
                        if (to_rank == -1 || to_rank == status.MPI_SOURCE) {  // данные у процесса, отправившего запрос?
                            MPI_Send(&status.MPI_SOURCE, 1, MPI_INT, status.MPI_SOURCE, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD);
                        } else {
                            int to_request[4] = {GET_DATA_RW, key, quantum, status.MPI_SOURCE};
                            MPI_Send(&to_rank, 1, MPI_INT, status.MPI_SOURCE, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD);  // отправление информации о том, с каким процессом
                                                                                                                             // нужно взаимодействовать для получения кванта
                            MPI_Send(to_request, 4, MPI_INT, to_rank, SEND_DATA_TO_HELPER, MPI_COMM_WORLD);  // отправление запроса вспомогательному потоку
                                                                                                             // процесса-рабочего о переслыке данных
                        }
                        break;
                    }
                    assert(quantum < (int)memory->quantum_owner.size());
                    assert(memory->quantum_owner[quantum].second != status.MPI_SOURCE);
                    if (memory->quantum_owner[quantum].second == -1) {  // данные ранее не запрашивались?
                        memory->quantum_owner[quantum] = {false, status.MPI_SOURCE};
                        MPI_Send(&status.MPI_SOURCE, 1, MPI_INT, status.MPI_SOURCE, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD);  // отправление информации о том, что процесс,
                                                                                                                                   // отправивший запрос, может забрать квант без
                                                                                                                                   // пересылок данных
                        break;
                    }  // empty
                    if (memory->quantum_owner[quantum].first) {  // данные готовы к пересылке?
                        int to_rank = memory->quantum_owner[quantum].second;
                        int to_request[4] = {GET_DATA_RW, key, quantum, status.MPI_SOURCE};
                        memory->quantum_owner[quantum] = {false, status.MPI_SOURCE};
                        MPI_Send(&to_rank, 1, MPI_INT, status.MPI_SOURCE, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD);  // отправление информации о том, с каким процессом
                                                                                                                         // нужно взаимодействовать для получения кванта
                        MPI_Send(to_request, 4, MPI_INT, to_rank, SEND_DATA_TO_HELPER, MPI_COMM_WORLD);  // отправление запроса вспомогательному потоку
                                                                                                         // процесса-рабочего о переслыке данных
                    } else {  // данные не готовы к пересылке (в данный момент пересылаются другому процессу)
                        memory->wait_quantums.push(quantum, status.MPI_SOURCE);
                    }
                }
                break;
            case SET_INFO:  // данные готовы для пересылки
                if (memory->mode[quantum] == READ_ONLY) {  // READ_ONLY mode
                    memory->owners[quantum].push_back(status.MPI_SOURCE); // процесс помещается в вектор процессов,
                                                                                 // которые могут пересылать данный квант другим процессам
                } else {  // READ_WRITE mode
                    assert(memory->quantum_owner[quantum].second == status.MPI_SOURCE);
                    assert(memory->quantum_owner[quantum].first == false);
                    memory->quantum_owner[quantum].first = true;
                    if (memory->wait_quantums.is_contain(quantum)) {  // есть процессы, ожидающие готовности кванта?
                        int source_rank = memory->wait_quantums.pop(quantum);
                        int to_rank = memory->quantum_owner[quantum].second;
                        memory->quantum_owner[quantum] = {false, source_rank};
                        int to_request[4] = {GET_DATA_RW, key, quantum, source_rank};
                        assert(source_rank != to_rank);
                        assert(to_rank > 0 && to_rank < size);
                        MPI_Send(&to_rank, 1, MPI_INT, source_rank, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD);  // отправление информации о том, с каким процессом
                                                                                                                   // нужно взаимодействовать для получения кванта
                        MPI_Send(to_request, 4, MPI_INT, to_rank, SEND_DATA_TO_HELPER, MPI_COMM_WORLD);  // отправление запроса вспомогательному потоку
                                                                                                         // процесса-рабочего о переслыке данных
                    }
                }
                break;
            case CHANGE_MODE:  // изменить режим работы с памятью
                memory_manager::memory[key]->num_of_change_mode_procs[quantum]++;
                if (memory_manager::memory[key]->num_of_change_mode_procs[quantum] == memory_manager::worker_size) {  // все процессы дошли до этапа изменения режима?
                    int ready = 1;
                    for (int i = 1; i < size; i++) {
                        MPI_Send(&ready, 1, MPI_INT, i, GET_PERMISSION_FOR_CHANGE_MODE, MPI_COMM_WORLD);  // информирование о смене режима и о том, что 
                                                                                                          // другие процессы могут продолжить выполнение программы дальше
                    }
                    memory_manager::memory[key]->num_of_change_mode_procs[quantum] = 0;
                    memory_manager::memory[key]->is_mode_changed[quantum] = true;
                    if (memory_manager::memory[key]->mode[quantum] == READ_ONLY) {
                        memory_manager::memory[key]->mode[quantum] = READ_WRITE;
                    } else {
                        memory_manager::memory[key]->mode[quantum] = READ_ONLY;
                    }
                }
                break;
        }
    }
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

void memory_manager::change_mode(int key, int quantum_index, int mode) {
    if (mode == memory[key]->mode[quantum_index])
        return;
    // информирование мастера о том, что данный процесс дошёл до этапа изменения режима работы с памятью
    int request[3] = {CHANGE_MODE, 0, -1};
    MPI_Send(request, 3, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);
    int is_ready;
    MPI_Status status;
    MPI_Recv(&is_ready, 1, MPI_INT, 0, GET_PERMISSION_FOR_CHANGE_MODE, MPI_COMM_WORLD, &status);  // после получения ответа данный процесс может продолжить выполнение
    memory[key]->mode[quantum_index] = mode;
    memory[key]->is_mode_changed[quantum_index] = true;
}

void memory_manager::finalize() {
    int cnt = 0;
    if(rank != 0) {
        int tmp = 1;
        MPI_Send(&tmp, 1, MPI_INT, 0, 23221, MPI_COMM_WORLD);
        MPI_Status status;
        MPI_Recv(&tmp, 1, MPI_INT, 0, 12455, MPI_COMM_WORLD, &status);
    } else {
        int tmp;
        for(int i = 1; i < size; i++) {
            MPI_Status status;
            MPI_Recv(&tmp, 1, MPI_INT, i, 23221, MPI_COMM_WORLD, &status);
        }
        for(int i = 1; i < size; i++) {
            MPI_Send(&tmp, 1, MPI_INT, i, 12455, MPI_COMM_WORLD);
        }
    }
    if (rank == 0) {
        int request[4] = {-1, -1, -1, -1};
        for(int i = 1; i < size; i++) {
            MPI_Send(request, 4, MPI_INT, i, SEND_DATA_TO_HELPER, MPI_COMM_WORLD);  // завершение работы вспомогательных потоков процессов рабочих
        }
    } else if (rank == 1) {
        int request[3] = {-1, -1, -1};
        MPI_Send(request, 3, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);  // завершение работы вспомогательного потока процесса-мастера
    }
    assert(helper_thr.joinable());
    helper_thr.join();
    // if (rank != 0) {
    //     for(int key = 0; key < (int)memory.size(); key++) {
    //         auto* memory = dynamic_cast<memory_line_worker*>(memory_manager::memory[key]);
    //         assert(memory[key].wait_quantums.size() == 0);
    //         if(!memory_manager::is_read_only_mode) {
    //             for(int quantum = 0; quantum < (int)memory[key].quantum_owner.size(); quantum++) {
    //                 assert(memory[key].quantum_owner[quantum].first == true);
    //             }
    //         }
    //         for(int i = 0; i < (int)memory->quantums.size(); i++) {
    //             assert(memory[key].quantums[i] == nullptr);
    //         }
    //     }
    // }
    MPI_Finalize();
}

int memory_manager::get_owner(int key, int quantum_index, int requesting_process) {
    int to_rank = -1;
    auto* memory = dynamic_cast<memory_line_master*>(memory_manager::memory[key]);
    for (int owner: memory->owners[quantum_index]) {  // поиск процесса, с которым мастер не взаимодействовал наиболее долгое время
        if (owner == requesting_process) {  // данные есть также и у процесса, который отправил запрос?
            to_rank = owner;
            break;
        }
    }
    to_rank = memory->owners[quantum_index].back();
    return to_rank;
}
