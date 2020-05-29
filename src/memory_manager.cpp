#include <thread>
#include <mpi.h>
#include <vector>
#include <climits>
#include <iostream>
#include <cassert>
#include <mutex>
#include "memory_manager.h"

// посылка: [операция; номер структуры, откуда требуются данные; требуемый номер элемента, кому требуется переслать объект;
//           значение элемента(для get_data = -1)]
// если номер структуры = -1, то завершение функции helper_thread
memory_manager mm;

void memory_manager::memory_manager_init(int argc, char**argv) {
    int provided = 0;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if(provided != MPI_THREAD_MULTIPLE) {
            std::cout << "Error: MPI thread support insufficient! required " << MPI_THREAD_MULTIPLE << " provided " << provided;
            abort();
    }
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    worker_rank = rank - 1;
    worker_size = size - 1;
    is_read_only_mode = false;
    num_of_change_mode_procs = 0;
    num_of_change_mode = 0;
    times.resize(size, LLONG_MIN);
    time = LLONG_MIN;
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
    // std::cout<<"(rank: "<<rank<<" num_elem: "<<number_of_elements<<" | create object begin)\n"<<std::flush;
    memory_line line;
    line.logical_size = number_of_elements;
    int num_of_quantums = (number_of_elements + QUANTUM_SIZE - 1) / QUANTUM_SIZE;
    if (rank == 0) {
        line.quantums_for_lock.resize(num_of_quantums);
        line.quantum_owner.resize(num_of_quantums);
        line.owners.resize(num_of_quantums);
        for (int i = 0; i < int(line.quantums_for_lock.size()); i++) {
            line.quantums_for_lock[i] = -1;
            line.quantum_owner[i] = {0, -1};
            line.owners[i] = std::vector<int>();
        }
    } else {
        line.mutexes.resize(num_of_quantums);
        line.quantums.resize(num_of_quantums);
        for (int i = 0; i < num_of_quantums; i++) {
            line.quantums[i] = nullptr;
            line.mutexes[i] = new std::mutex();
        }
    }
    line.num_change_mode.resize(num_of_quantums, 0);

    memory.emplace_back(line);
    MPI_Barrier(MPI_COMM_WORLD);
    // std::cout<<"(rank: "<<rank<<" num_elem: "<<number_of_elements<<" | create object end)\n"<<std::flush;
    return memory.size()-1;
}

int memory_manager::get_data(int key, int index_of_element) {
    int num_quantum = get_quantum_index(index_of_element);
    auto& quantum = mm.memory[key].quantums[num_quantum];
    bool f = false;
    if (!is_read_only_mode)   // если read_write mode, то используем мьютекс на данный квант
        mm.memory[key].mutexes[num_quantum]->lock();
    if (quantum != nullptr) {  // на данном процессе есть квант?
        if (mm.memory[key].num_change_mode[num_quantum] == mm.num_of_change_mode) {  // не было изменения режима? (данные актуальны?)
            int elem = quantum[index_of_element%QUANTUM_SIZE];
            if (!is_read_only_mode)
                mm.memory[key].mutexes[num_quantum]->unlock();
            return elem;  // элемент возвращается без обращения к мастеру
        }
    } else {
        quantum = new int[QUANTUM_SIZE];  // выделение памяти
    }
    int request[3] = {GET_INFO, key, num_quantum};  // обращение к мастеру: {тип операции, идентификатор вектора, номер кванта}
    MPI_Send(request, 3, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);
    int to_rank = -2;
    MPI_Status status;
    MPI_Recv(&to_rank, 1, MPI_INT, 0, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD, &status);  
    mm.memory[key].num_change_mode[num_quantum] = mm.num_of_change_mode;  
    if (is_read_only_mode && to_rank == rank) {
        return quantum[index_of_element%QUANTUM_SIZE];
    }
    if (to_rank != rank) { // если to_rank == rank, то данные можно отдать процессу, но он должен оповестить процесс-мастер при завершении работы с квантом
        assert(quantum != nullptr);
        assert(to_rank > 0 && to_rank < size);
        MPI_Recv(quantum, QUANTUM_SIZE, MPI_INT, to_rank, GET_DATA_FROM_HELPER, MPI_COMM_WORLD, &status);
    }
    request[0] = SET_INFO;
    MPI_Send(request, 3, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);
    int elem = quantum[index_of_element%QUANTUM_SIZE];
    if (!is_read_only_mode)
        mm.memory[key].mutexes[num_quantum]->unlock();
    return elem;
}

void memory_manager::set_data(int key, int index_of_element, int value) {
    // std::cout<<index_of_element<<"!\n"<<std::flush;
    // std::cout<<"(rank: "<<rank<<" index: "<<index_of_element<<" set_data | 1)\n"<<std::flush;
    assert(key >= 0 && key < (int)mm.memory.size());
    if(is_read_only_mode) {
        throw -1;
    }
    int num_quantum = get_quantum_index(index_of_element);
    assert(index_of_element >= 0 && index_of_element < (int)mm.memory[key].logical_size);
    assert(num_quantum >= 0 && num_quantum < (int)mm.memory[key].quantums.size());
    auto& quantum = mm.memory[key].quantums[num_quantum];
    assert(num_quantum >= 0 && num_quantum < (int)mm.memory[key].mutexes.size());
    mm.memory[key].mutexes[num_quantum]->lock();
    assert((index_of_element%QUANTUM_SIZE) >= 0);
    if (quantum != nullptr) {
        if (mm.memory[key].num_change_mode[num_quantum] == mm.num_of_change_mode) {
            quantum[index_of_element%QUANTUM_SIZE] = value;
            mm.memory[key].mutexes[num_quantum]->unlock();
            // std::cout<<"(rank: "<<rank<<" index: "<<index_of_element<<" set_data | not nullptr)\n"<<std::flush;
            return;
        }
    } else {
        // std::cout<<"1$$$\n"<<std::flush<<"\n";
        quantum = new int[QUANTUM_SIZE];
        // std::cout<<"2$$$\n"<<std::flush<<"\n";
    }
    int request[3] = {GET_INFO, key, num_quantum};
    // std::cout<<"3$$$\n"<<std::flush<<"\n";
    MPI_Send(request, 3, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);
    // std::cout<<"4$$$\n"<<std::flush<<"\n";
    int to_rank = -2;
    MPI_Status status;
    // std::cout<<"5$$$\n"<<std::flush<<"\n";
    MPI_Recv(&to_rank, 1, MPI_INT, 0, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD, &status);
    // std::cout<<"6$$$\n"<<std::flush<<"\n";
    assert(num_quantum >= 0 && num_quantum < (int)mm.memory[key].num_change_mode.size());
    mm.memory[key].num_change_mode[num_quantum] = mm.num_of_change_mode;
    if (to_rank != rank) {  // если to_rank == rank, то данные можно отдать процессу, но он должен оповестить процесс-мастер при завершении работы с квантом
        assert(quantum != nullptr);
        assert(to_rank > 0 && to_rank < size);
        // std::cout<<"(rank: "<<rank<<" index: "<<index_of_element<<" set_data | nullptr get from another process)\n"<<std::flush;
        MPI_Recv(quantum, QUANTUM_SIZE, MPI_INT, to_rank, GET_DATA_FROM_HELPER, MPI_COMM_WORLD, &status);
    } else {
        // std::cout<<"(rank: "<<rank<<" index: "<<index_of_element<<" set_data | nullptr first use)\n"<<std::flush;
    }
    // std::cout<<"7$$$\n"<<std::flush<<"\n";
    request[0] = SET_INFO;
    MPI_Send(request, 3, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);
    // std::cout<<"8$$$\n"<<std::flush<<"\n";
    assert(quantum != nullptr);
    quantum[index_of_element%QUANTUM_SIZE] = value;
    // std::cout<<"("<<rank<<" "<<index_of_element<<" set_data | nullptr set info)\n"<<std::flush;
    mm.memory[key].mutexes[num_quantum]->unlock();
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
        if(request[0] == -1 && request[1] == -1 && request[2] == -1 && request[3] == -1) {
            for (int key = 0; key < int(mm.memory.size()); key++) {
                for(int i = 0; i < int(mm.memory[key].quantums.size()); i++) {
                    if(mm.memory[key].quantums[i] != nullptr) {
                        delete[] mm.memory[key].quantums[i];
                        mm.memory[key].quantums[i] = nullptr;
                    }
                    delete mm.memory[key].mutexes[i];
                }
            }
            break;
        }
        int key = request[1], quantum_index = request[2], to_rank = request[3];
        // std::cout<<"(rank: "<<rank<<" quantum: "<<quantum_index<<" to_rank: "<<to_rank<<" worker)\n"<<std::flush;
        assert(mm.memory[key].quantums[quantum_index] != nullptr);
        assert(to_rank > 0 && to_rank < size);
        assert(key >= 0 && key < (int)mm.memory.size());
        assert(quantum_index >= 0 && quantum_index < (int)mm.memory[key].quantums.size());
        switch(request[0]) {
            case GET_DATA_R:
                MPI_Send(mm.memory[key].quantums[quantum_index], QUANTUM_SIZE,
                                        MPI_INT, to_rank, GET_DATA_FROM_HELPER, MPI_COMM_WORLD);
                break;
            case GET_DATA_RW:
                assert(quantum_index >= 0 && quantum_index < (int)mm.memory[key].mutexes.size());
                mm.memory[key].mutexes[quantum_index]->lock();
                MPI_Send(mm.memory[key].quantums[quantum_index], QUANTUM_SIZE,
                                        MPI_INT, to_rank, GET_DATA_FROM_HELPER, MPI_COMM_WORLD);
                delete[] mm.memory[key].quantums[quantum_index];
                mm.memory[key].quantums[quantum_index] = nullptr;
                mm.memory[key].mutexes[quantum_index]->unlock();
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
        if (request[0] == -1 && request[1] == -1 && request[2] == -1) {
            // std::cout<<"("<<rank<<" end of master)\n"<<std::flush;
            break;
        }
        int key = request[1], quantum = request[2];
        // std::cout<<key<<" "<<" "<<quantum<<" "<<request[0]<<" "<<mm.memory.size()<<" "<<(key >= 0 && key < (int)mm.memory.size())<<"!!!!!!!!!!!!!!!!!!!!\n"<<std::flush;
        if (request[0] != CHANGE_MODE) {
            assert(key >= 0 && key < (int)mm.memory.size());
            assert(quantum >= 0 && quantum < (int)mm.memory[key].quantum_owner.size());
        }
        switch(request[0]) {
            case LOCK_READ:
                if (mm.memory[key].quantums_for_lock[quantum] == -1) {
                    int to_rank = status.MPI_SOURCE;
                    int tmp = 1;
                    mm.memory[key].quantums_for_lock[quantum] = status.MPI_SOURCE;
                    MPI_Send(&tmp, 1, MPI_INT, to_rank, GET_DATA_FROM_MASTER_HELPER_LOCK, MPI_COMM_WORLD);
                } else {
                    if (mm.memory[key].wait_locks.find(quantum) == mm.memory[key].wait_locks.end())
                        mm.memory[key].wait_locks.insert({quantum, std::queue<int>{}});
                    mm.memory[key].wait_locks[quantum].push(status.MPI_SOURCE);
                }
                break;
            case UNLOCK:
                if (mm.memory[key].quantums_for_lock[quantum] == status.MPI_SOURCE) {
                    mm.memory[key].quantums_for_lock[quantum] = -1;
                    if (mm.memory[key].wait_locks.find(quantum) != mm.memory[key].wait_locks.end()) {
                        int to_rank = mm.memory[key].wait_locks[quantum].front();
                        mm.memory[key].wait_locks[quantum].pop();
                        mm.memory[key].quantums_for_lock[quantum] = to_rank;
                        if (mm.memory[key].wait_locks[quantum].size() == 0)
                            mm.memory[key].wait_locks.erase(quantum);
                        int tmp = 1;
                        MPI_Send(&tmp, 1, MPI_INT, to_rank, GET_DATA_FROM_MASTER_HELPER_LOCK, MPI_COMM_WORLD);
                    }
                }
                break;
            case GET_INFO:                
                if (mm.is_read_only_mode) {
                    // std::cout<<"(quantum: "<<quantum<<" source: "<<status.MPI_SOURCE<<" master read_only)\n"<<std::flush;
                    assert(quantum < (int)mm.memory[key].num_change_mode.size());
                    assert(quantum < (int)mm.memory[key].quantum_owner.size());
                    assert(quantum < (int)mm.memory[key].owners.size());
                    if (mm.memory[key].num_change_mode[quantum] != mm.num_of_change_mode) {  // был переход между режимами?
                        // std::cout<<"(quantum: "<<quantum<<" source: "<<status.MPI_SOURCE<<" master read_only first change)\n"<<std::flush;
                        if (mm.memory[key].quantum_owner[quantum].second == -1)
                            throw -1;
                        assert(mm.memory[key].quantum_owner[quantum].first == true);
                        mm.memory[key].owners[quantum].clear();
                        mm.memory[key].owners[quantum].push_back(mm.memory[key].quantum_owner[quantum].second);
                        mm.memory[key].num_change_mode[quantum] = mm.num_of_change_mode;
                        int to_rank = mm.memory[key].owners[quantum][0];
                        if (to_rank == status.MPI_SOURCE) {
                            MPI_Send(&to_rank, 1, MPI_INT, status.MPI_SOURCE, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD);
                            // std::cout<<"(quantum: "<<quantum<<" source: "<<status.MPI_SOURCE<<" master read_only first change to_rank = status.MPI_SOURCE)\n"<<std::flush;
                            break;
                        }
                    }
                    if (mm.memory[key].owners[quantum].empty()) {
                            throw -1;  // ?
                    }
                    int to_rank = -2;
                    long long minn = mm.time+1;
                    for (int owner: mm.memory[key].owners[quantum]) {
                        assert(owner < (int)mm.times.size());
                        if (owner == status.MPI_SOURCE) {
                            to_rank = owner;
                            break;
                        }
                        if (mm.times[owner] < minn) {
                            to_rank = owner;
                            minn = mm.times[owner];
                        }
                    }
                    // std::cout<<"(quantum: "<<quantum<<" source: "<<status.MPI_SOURCE<<" to_rank: "<<to_rank<<" "<<" master read_only who is chosen)\n"<<std::flush;
                    assert(to_rank > 0 && to_rank < size);
                    mm.times[to_rank] = mm.time;
                    mm.times[status.MPI_SOURCE] = mm.time++;
                    int to_request[4] = {GET_DATA_R, key, quantum, status.MPI_SOURCE};
                    MPI_Send(&to_rank, 1, MPI_INT, status.MPI_SOURCE, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD);
                    if(to_rank != status.MPI_SOURCE) {
                        MPI_Send(to_request, 4, MPI_INT, to_rank, SEND_DATA_TO_HELPER, MPI_COMM_WORLD);
                    } else {
                        mm.memory[key].owners[quantum].push_back(to_rank);
                    }
                } else {  // read_write mode
                    // std::cout<<"(quantum: "<<quantum<<" source: "<<status.MPI_SOURCE<<" master read_write mode)\n"<<std::flush;
                    assert(quantum < (int)mm.memory[key].num_change_mode.size());
                    if (mm.memory[key].num_change_mode[quantum] != mm.num_of_change_mode) {  // был переход между режимами?
                        // std::cout<<"(quantum: "<<quantum<<" source: "<<status.MPI_SOURCE<<" master read_write first change)\n"<<std::flush;
                        long long minn = mm.time+1;
                        int to_rank = -1;
                        for (int owner: mm.memory[key].owners[quantum]) {
                            assert(owner < (int)mm.times.size());
                            if (owner == status.MPI_SOURCE) {
                                to_rank = owner;
                                break;
                            }
                            if (mm.times[owner] < minn) {
                                to_rank = owner;
                                minn = mm.times[owner];
                            }
                        }
                        mm.memory[key].num_change_mode[quantum] = mm.num_of_change_mode;
                        mm.memory[key].quantum_owner[quantum] = {false, status.MPI_SOURCE};
                        if (to_rank == -1 || to_rank == status.MPI_SOURCE) {
                            MPI_Send(&status.MPI_SOURCE, 1, MPI_INT, status.MPI_SOURCE, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD);
                            // std::cout<<"(quantum: "<<quantum<<" source: "<<status.MPI_SOURCE<<" master read_write first change to_rank = status.MPI_SOURCE or first use)\n"<<std::flush;
                        } else {
                            int to_request[4] = {GET_DATA_RW, key, quantum, status.MPI_SOURCE};
                            MPI_Send(&to_rank, 1, MPI_INT, status.MPI_SOURCE, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD);
                            MPI_Send(to_request, 4, MPI_INT, to_rank, SEND_DATA_TO_HELPER, MPI_COMM_WORLD);
                            // std::cout<<"(quantum: "<<quantum<<" source: "<<status.MPI_SOURCE<<" to_rank: "<<to_rank<<" "<<" master read_write first change who is chosen)\n"<<std::flush;
                        }
                        break;
                    }
                    assert(quantum < (int)mm.memory[key].quantum_owner.size());
                    assert(mm.memory[key].quantum_owner[quantum].second != status.MPI_SOURCE);
                    if (mm.memory[key].quantum_owner[quantum].second == -1) {
                        mm.memory[key].quantum_owner[quantum] = {false, status.MPI_SOURCE};
                        MPI_Send(&status.MPI_SOURCE, 1, MPI_INT, status.MPI_SOURCE, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD);
                        // std::cout<<"(quantum: "<<quantum<<" source: "<<status.MPI_SOURCE<<" "<<" master read_write first use)\n"<<std::flush;
                        break;
                    }  // empty
                    if (mm.memory[key].quantum_owner[quantum].first) {  // queue is empty, can get a quantum
                        int to_rank = mm.memory[key].quantum_owner[quantum].second;
                        int to_request[4] = {GET_DATA_RW, key, quantum, status.MPI_SOURCE};
                        mm.memory[key].quantum_owner[quantum] = {false, status.MPI_SOURCE};
                        MPI_Send(&to_rank, 1, MPI_INT, status.MPI_SOURCE, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD);
                        MPI_Send(to_request, 4, MPI_INT, to_rank, SEND_DATA_TO_HELPER, MPI_COMM_WORLD);
                        // std::cout<<"(quantum: "<<quantum<<" source: "<<status.MPI_SOURCE<<" to_rank: "<<to_rank<<" master read_write who is chosen)\n"<<std::flush;
                    } else {  // quantum is not ready yet
                        if (mm.memory[key].wait_quantums.find(quantum) == mm.memory[key].wait_quantums.end())
                            mm.memory[key].wait_quantums.insert({quantum, std::queue<int>()});
                        mm.memory[key].wait_quantums[quantum].push(status.MPI_SOURCE);
                        // std::cout<<"(quantum: "<<quantum<<" source: "<<status.MPI_SOURCE<<" master read_write go to queue)\n"<<std::flush;
                    }
                }
                break;
            case SET_INFO:
                if (mm.is_read_only_mode) {
                    // std::cout<<"(quantum: "<<quantum<<" source: "<<status.MPI_SOURCE<<" master read_only set_info)\n"<<std::flush;
                    mm.memory[key].owners[quantum].push_back(status.MPI_SOURCE);
                } else {  // read_write mode
                    // std::cout<<"(quantum: "<<quantum<<" source: "<<status.MPI_SOURCE<<" master read_write set_info)\n"<<std::flush;
                    assert(mm.memory[key].quantum_owner[quantum].second == status.MPI_SOURCE);
                    assert(mm.memory[key].quantum_owner[quantum].first == false);
                    mm.memory[key].quantum_owner[quantum].first = true;
                    if (mm.memory[key].wait_quantums.find(quantum) != mm.memory[key].wait_quantums.end()) {
                        std::queue<int>& wait_queue = mm.memory[key].wait_quantums[quantum];
                        int source_rank = wait_queue.front();
                        wait_queue.pop();
                        if (wait_queue.size() == 0) {
                            mm.memory[key].wait_quantums.erase(quantum);
                            assert(mm.memory[key].wait_quantums.find(quantum) == mm.memory[key].wait_quantums.end());
                        }
                        int to_rank = mm.memory[key].quantum_owner[quantum].second;
                        // std::cout<<"(quantum: "<<quantum<<" source: "<<source_rank<<" master read_write get from queue)\n"<<std::flush;
                        mm.memory[key].quantum_owner[quantum] = {false, source_rank};
                        int to_request[4] = {GET_DATA_RW, key, quantum, source_rank};
                        assert(source_rank != to_rank);
                        assert(to_rank > 0 && to_rank < size);
                        MPI_Send(&to_rank, 1, MPI_INT, source_rank, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD);
                        MPI_Send(to_request, 4, MPI_INT, to_rank, SEND_DATA_TO_HELPER, MPI_COMM_WORLD);
                        // std::cout<<"(quantum: "<<quantum<<" source: "<<source_rank<<" to_rank: "<<to_rank<<" master read_write from queue who is chosen)\n"<<std::flush;
                    }
                }
                break;
            case CHANGE_MODE:
                mm.num_of_change_mode_procs++;
                if (mm.num_of_change_mode_procs == mm.worker_size) {
                    mm.time = 0;
                    mm.is_read_only_mode = request[1];
                    int ready = 1;
                    for (int i = 1; i < size; i++) {
                        MPI_Send(&ready, 1, MPI_INT, i, GET_PERMISSION_FOR_CHANGE_MODE, MPI_COMM_WORLD);
                        mm.times[i] = 0;
                    }
                    mm.num_of_change_mode_procs = 0;
                    mm.num_of_change_mode++;
                }
                break;
        }
    }
}

void memory_manager::set_lock(int key, int quantum_index) {
    int request[] = {LOCK_READ, key, quantum_index};
    MPI_Send(request, 3, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);
    int ans;
    MPI_Status status;
    MPI_Recv(&ans, 1, MPI_INT, 0, GET_DATA_FROM_MASTER_HELPER_LOCK, MPI_COMM_WORLD, &status);
}

void memory_manager::unset_lock(int key, int quantum_index) {
    int request[3] = {UNLOCK, key, quantum_index};
    MPI_Send(request, 3, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);
}

void memory_manager::change_mode(int mode) {
    if (mode == READ_ONLY && is_read_only_mode ||
        mode == READ_WRITE && !is_read_only_mode)
        return;
    if (mode == READ_ONLY) {
        int request[3] = {CHANGE_MODE, 1, -1};
        MPI_Send(request, 3, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);
    } else if (mode == READ_WRITE) {
        int request[3] = {CHANGE_MODE, 0, -1};
        MPI_Send(request, 3, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);
    }
    int is_ready;
    MPI_Status status;
    MPI_Recv(&is_ready, 1, MPI_INT, 0, GET_PERMISSION_FOR_CHANGE_MODE, MPI_COMM_WORLD, &status);
    if(mode == READ_ONLY)
        is_read_only_mode = true;
    else
        is_read_only_mode = false;
    mm.num_of_change_mode++;
}

void memory_manager::finalize() {
    // std::cout<<"(rank: "<<rank<<" | finalize begin)\n"<<std::flush;
    int cnt = 0;
    if(rank != 0) {
        int tmp = 1;
        // std::cout<<"(rank: "<<rank<<" | finalize send)\n"<<std::flush;
        MPI_Send(&tmp, 1, MPI_INT, 0, 23221, MPI_COMM_WORLD);
        MPI_Status status;
        // std::cout<<"(rank: "<<rank<<" | finalize begin recv)\n"<<std::flush;
        MPI_Recv(&tmp, 1, MPI_INT, 0, 12455, MPI_COMM_WORLD, &status);
        // std::cout<<"(rank: "<<rank<<" | finalize begin end recv)\n"<<std::flush;
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
    // std::cout<<"( rank: "<<rank<<" | finalize go from barrier)\n"<<std::flush;
    if (rank == 0) {
        int request[4] = {-1, -1, -1, -1};
        for(int i = 1; i < size; i++) {
            MPI_Send(request, 4, MPI_INT, i, SEND_DATA_TO_HELPER, MPI_COMM_WORLD);
        }
    } else if (rank == 1) {
        int request[3] = {-1, -1, -1};
        MPI_Send(request, 3, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);
    }
    assert(helper_thr.joinable());
    helper_thr.join();
    if (rank != 0) {
        for(int key = 0; key < (int)memory.size(); key++) {
            assert(memory[key].wait_quantums.size() == 0);
            if(!mm.is_read_only_mode) {
                for(int quantum = 0; quantum < (int)memory[key].quantum_owner.size(); quantum++) {
                    assert(memory[key].quantum_owner[quantum].first == true);
                }
            }
            for(int i = 0; i < (int)mm.memory[key].quantums.size(); i++) {
                assert(memory[key].quantums[i] == nullptr);
            }
        }
    }
    MPI_Finalize();
    // std::cout<<"(rank: "<<rank<<" | finalize end)\n"<<std::flush;
}

// memory_manager::~memory_manager() {
    // int request[2] = {-1, -1};
    // if(rank == 0) {
    //     for(int i = 0; i < size; i++) {
    //         std::cout<<"MPI_Send to rank "<<rank<<" from destructor\n";
    //         MPI_Send(request, 2, MPI_INT, 0, SEND_DATA_TO_HELPER, MPI_COMM_WORLD);
    //     }
    // }
    // if(helper_thr.joinable())   
    //     helper_thr.join();
    // std::cout<<"join ended\n";
    // MPI_Finalize();
    // std::cout<<"this is destructor\n";
// }