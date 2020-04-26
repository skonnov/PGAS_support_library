#include <thread>
#include <mpi.h>
#include <vector>
#include <iostream>
#include <cassert>
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
    if(rank == 0) {
        helper_thr = std::thread(master_helper_thread);
    } else {
        helper_thr = std::thread(worker_helper_thread);
    }
    is_read_only_mode = false;
    num_of_change_mode_procs = 0;
    num_of_change_mode = 0;
    // создание своего типа для пересылки посылок ???
}

int memory_manager::create_object(int number_of_elements) {
    memory_line line;
    line.logical_size = number_of_elements;
    int num_of_quantums = (number_of_elements + QUANTUM_SIZE - 1) / QUANTUM_SIZE;
    if (rank == 0) {
        line.quantums_for_lock.resize(num_of_quantums);
        line.quantum_owner.resize(num_of_quantums);
        line.owners.resize(num_of_quantums);
        line.times.resize(size, LLONG_MIN);
        line.time = LLONG_MIN;
        for (int i = 0; i < int(line.quantums_for_lock.size()); i++) {
            line.quantums_for_lock[i] = -1;
            line.quantum_owner[i] = {0, -1};
            line.owners[i] = std::vector<int>();
        }
    } else {
        line.quantums.resize((number_of_elements + QUANTUM_SIZE - 1) / QUANTUM_SIZE);
        for (int i = 0; i < int(line.quantums.size()); i++) {
            line.quantums[i] = nullptr;
        }
    }
    line.num_change_mode.resize(num_of_quantums, 0);
    memory.push_back(line);
    MPI_Barrier(MPI_COMM_WORLD);
    return memory.size()-1;
}

// void memory_manager::delete_object(int* object) { 
//     auto tmp = map_pointer_to_int.find(object);
//     if(tmp == map_pointer_to_int.end())
//         throw -1;
//     auto tmp2 = map_int_to_pointer.find(tmp->second);  // ???
//     if(tmp2 == map_int_to_pointer.end())
//         throw -2;
//     map_int_to_pointer.erase(tmp2);
//     map_pointer_to_int.erase(tmp);
// }


// int memory_manager::get_size_of_portion(int key) {
//     return memory[key].vector_size;
// }


int memory_manager::get_data(int key, int index_of_element) {
    int num_quantum = get_quantum_index(index_of_element);
    auto& quantum = mm.memory[key].quantums[num_quantum];
    if (quantum != nullptr) {
        if (mm.memory[key].num_change_mode[num_quantum] == mm.num_of_change_mode)
            return quantum[index_of_element%QUANTUM_SIZE];
    } else {
        quantum = new int[QUANTUM_SIZE];
    }
    int request[3] = {GET_INFO, key, num_quantum};
    MPI_Send(request, 3, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);
    int to_rank = -1;
    MPI_Status status;
    MPI_Recv(&to_rank, 1, MPI_INT, 0, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD, &status);
    mm.memory[key].num_change_mode[num_quantum] = mm.num_of_change_mode;
    if (to_rank == -1)  // данные уже у процесса, он может обрабатывать их дальше
        return quantum[index_of_element%QUANTUM_SIZE];
    if (to_rank != rank)  // если to_rank == rank, то данные можно отдать процессу, но он должен оповестить процесс-мастер при завершении работы с квантом
        MPI_Recv(quantum, QUANTUM_SIZE, MPI_INT, to_rank, GET_DATA_FROM_HELPER, MPI_COMM_WORLD, &status);
    request[0] = SET_INFO;
    MPI_Send(request, 3, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);
    return quantum[index_of_element%QUANTUM_SIZE];
}

void memory_manager::set_data(int key, int index_of_element, int value) {
    if(is_read_only_mode) {
        throw -1;
    }
    int num_quantum = get_quantum_index(index_of_element);
    auto& quantum = mm.memory[key].quantums[num_quantum];
    if (quantum != nullptr) {
        if (mm.memory[key].num_change_mode[num_quantum] == mm.num_of_change_mode) {
            quantum[index_of_element%QUANTUM_SIZE] = value;
            return;
        }
    } else {
        quantum = new int[QUANTUM_SIZE];
    }
    quantum = new int[QUANTUM_SIZE];
    int request[3] = {GET_INFO, key, num_quantum};
    MPI_Send(request, 3, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);
    int to_rank = -1;
    MPI_Status status;
    MPI_Recv(&to_rank, 1, MPI_INT, 0, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD, &status);
    mm.memory[key].num_change_mode[num_quantum] = mm.num_of_change_mode;
    if (to_rank == -1) {  // данные уже у процесса, он может обрабатывать их дальше
        quantum[index_of_element%QUANTUM_SIZE] = value;
        return;
    }
    if (to_rank != rank)  // если to_rank == rank, то данные можно отдать процессу, но он должен оповестить процесс-мастер при завершении работы с квантом
        MPI_Recv(quantum, QUANTUM_SIZE, MPI_INT, to_rank, GET_DATA_FROM_HELPER, MPI_COMM_WORLD, &status);
    request[0] = SET_INFO;
    MPI_Send(request, 3, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);
    quantum[index_of_element%QUANTUM_SIZE] = value;
}

void memory_manager::copy_data(int key_from, int key_to) {
    memory[key_to] = memory[key_from];
}

// int memory_manager::get_data_by_index_on_process(int key, int index) {
//     return memory[key].vector[index];
// }

// void memory_manager::set_data_by_index_on_process(int key, int index, int value) {
//     if(is_read_only_mode)
//         throw -1;
//     memory[key].vector[index] = value;
// }

int memory_manager::get_number_of_process(int key, int index) {
    int number_proc;
    int quantum_index = get_quantum_index(index);
    int tmp1 = int((memory[key].logical_size + QUANTUM_SIZE - 1)/QUANTUM_SIZE)%worker_size;
    int tmp2 = int((memory[key].logical_size + QUANTUM_SIZE - 1)/QUANTUM_SIZE)/worker_size;
    if(quantum_index < tmp1*(tmp2+1)) {
        number_proc = quantum_index/(tmp2+1);
    } else {
        int tmp = quantum_index - tmp1*(tmp2+1);
        number_proc = tmp1 + tmp/tmp2;
    }
    number_proc += 1;
    return number_proc;
}

int memory_manager::get_number_of_element(int key, int index) {
    int number_elem, quantum_number_elem;
    int quantum_index = get_quantum_index(index);
    int tmp1 = int((memory[key].logical_size + QUANTUM_SIZE - 1)/QUANTUM_SIZE)%worker_size;
    int tmp2 = int((memory[key].logical_size + QUANTUM_SIZE - 1)/QUANTUM_SIZE)/worker_size;
    if(quantum_index < tmp1*(tmp2+1)) {
        quantum_number_elem = quantum_index%(tmp2+1);
    } else {
        int tmp = quantum_index - tmp1*(tmp2+1);
        quantum_number_elem = tmp%tmp2;
    }
    number_elem = quantum_number_elem*QUANTUM_SIZE + index%QUANTUM_SIZE;
    return number_elem;
}

int memory_manager::get_global_index_of_element(int key, int index, int process) {
    if(process == 0)
        throw -2;
    int number_elem;
    process -= 1;
    int num_of_quantums = (memory[key].logical_size + QUANTUM_SIZE-1)/QUANTUM_SIZE;
    if(process < num_of_quantums%worker_size) {
        number_elem = process*(num_of_quantums/worker_size+1)*QUANTUM_SIZE+index;
    } else {
        number_elem = (num_of_quantums/worker_size+1)*QUANTUM_SIZE*(num_of_quantums%worker_size) +
                                    + (process-num_of_quantums%worker_size)*(num_of_quantums/worker_size)*QUANTUM_SIZE + index;
    }
    return number_elem;
}

int memory_manager::get_quantum_index(int index) {
    return index/QUANTUM_SIZE;
}


void worker_helper_thread() {
    int request[4];
    MPI_Status status;
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    while(true) {
        MPI_Recv(request, 4, MPI_INT, MPI_ANY_SOURCE, SEND_DATA_TO_HELPER, MPI_COMM_WORLD, &status);
        if(request[0] == -1) {
            for (int key = 0; key < int(mm.memory.size()); key++) {
                for(int i = 0; i < int(mm.memory[key].quantums.size()); i++) {
                    if(mm.memory[key].quantums[i] != nullptr) {
                        delete[] mm.memory[key].quantums[i];
                        mm.memory[key].quantums[i] = nullptr;
                    }
                }
            }
            break;
        }
        int key = request[1], quantum_index = request[2], to_rank = request[3];
        switch(request[0]) {
            case GET_DATA_R:
                MPI_Send(mm.memory[key].quantums[quantum_index], QUANTUM_SIZE,
                                        MPI_INT, to_rank, GET_DATA_FROM_HELPER, MPI_COMM_WORLD);
                break;
            case GET_DATA_RW:
                MPI_Send(mm.memory[key].quantums[quantum_index], QUANTUM_SIZE,
                                        MPI_INT, to_rank, GET_DATA_FROM_HELPER, MPI_COMM_WORLD);
                delete[] mm.memory[key].quantums[quantum_index];
                mm.memory[key].quantums[quantum_index] = nullptr;
                break;
        }
    }
}

void master_helper_thread() {
    int request[3];
    MPI_Status status;
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    while(true) {
        MPI_Recv(&request, 3, MPI_INT, MPI_ANY_SOURCE, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD, &status);
        if (request[0] == -1) {
            break;
        }
        int key = request[1], quantum = request[2];
        switch(request[0]) {
            case LOCK_READ:
            case LOCK_WRITE:
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
                    assert(quantum < (int)mm.memory[key].num_change_mode.size());
                    assert(quantum < (int)mm.memory[key].quantum_owner.size());
                    assert(quantum < (int)mm.memory[key].owners.size());
                    if (mm.memory[key].num_change_mode[quantum] != mm.num_of_change_mode) {  // был переход между режимами?
                        if (mm.memory[key].quantum_owner[quantum].second == -1)
                            throw -1;
                        assert(mm.memory[key].quantum_owner[quantum].first == true);
                        mm.memory[key].owners[quantum].clear();
                        mm.memory[key].owners[quantum].push_back(mm.memory[key].quantum_owner[quantum].second);
                        mm.memory[key].num_change_mode[quantum] = mm.num_of_change_mode;
                        if (mm.memory[key].owners[quantum][0] == status.MPI_SOURCE) {
                            int tmp = -1;
                            MPI_Send(&tmp, 1, MPI_INT, status.MPI_SOURCE, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD);
                            break;
                        }
                    }
                    if (mm.memory[key].owners[quantum].empty()) {
                            throw -1;  // ?
                    }
                    int to_rank = -1;
                    long long minn = mm.memory[key].time+1;
                    for (int owner: mm.memory[key].owners[quantum]) {
                        assert(owner < (int)mm.memory[key].times.size());
                        if (mm.memory[key].times[owner] < minn) {
                            to_rank = owner;
                            minn = mm.memory[key].times[owner];
                        }
                    }
                    assert(to_rank > 0 && to_rank < size);
                    mm.memory[key].times[to_rank] = mm.memory[key].time;
                    mm.memory[key].times[status.MPI_SOURCE] = mm.memory[key].time++;
                    int to_request[4] = {GET_DATA_R, key, quantum, status.MPI_SOURCE};
                    if (to_rank == status.MPI_SOURCE)
                        to_rank = -1;
                    MPI_Send(&to_rank, 1, MPI_INT, status.MPI_SOURCE, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD);
                    if(to_rank != -1)
                        MPI_Send(to_request, 4, MPI_INT, to_rank, SEND_DATA_TO_HELPER, MPI_COMM_WORLD);
                } else {  // read_write mode
                    assert(quantum < (int)mm.memory[key].num_change_mode.size());
                    if (mm.memory[key].num_change_mode[quantum] != mm.num_of_change_mode) {  // был переход между режимами?
                        long long minn = mm.memory[key].time+1;
                        int to_rank = -1;
                        for (int owner: mm.memory[key].owners[quantum]) {
                            if (owner == status.MPI_SOURCE) {
                                to_rank = owner;
                                break;
                            }
                            if (mm.memory[key].times[owner] < minn) {
                                to_rank = owner;
                                minn = mm.memory[key].times[owner];
                            }
                        }
                        if (to_rank == -1) {
                            mm.memory[key].quantum_owner[quantum] = {false, -1};
                        } else {
                            mm.memory[key].quantum_owner[quantum] = {true, to_rank};
                        }
                        mm.memory[key].num_change_mode[quantum] = mm.num_of_change_mode;
                        if (to_rank == status.MPI_SOURCE) {
                            int tmp = -1;
                            MPI_Send(&tmp, 1, MPI_INT, status.MPI_SOURCE, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD);
                            break;
                        }
                    }
                    assert(quantum < (int)mm.memory[key].quantum_owner.size());
                    if (mm.memory[key].quantum_owner[quantum].second == -1 ||
                                    mm.memory[key].quantum_owner[quantum].second == status.MPI_SOURCE) {
                        mm.memory[key].quantum_owner[quantum] = {false, status.MPI_SOURCE};
                        MPI_Send(&status.MPI_SOURCE, 1, MPI_INT, status.MPI_SOURCE, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD);
                        break;
                    }  // empty
                    if (mm.memory[key].quantum_owner[quantum].first) {  // queue is empty, can get a quantum
                        int to_rank = mm.memory[key].quantum_owner[quantum].second;
                        int to_request[4] = {GET_DATA_RW, key, quantum, status.MPI_SOURCE};
                        mm.memory[key].quantum_owner[quantum] = {false, status.MPI_SOURCE};
                        MPI_Send(&to_rank, 1, MPI_INT, status.MPI_SOURCE, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD);
                        MPI_Send(to_request, 4, MPI_INT, to_rank, SEND_DATA_TO_HELPER, MPI_COMM_WORLD);
                    } else {  // quantum is not ready yet
                        if (mm.memory[key].wait_quantums.find(quantum) == mm.memory[key].wait_quantums.end())
                            mm.memory[key].wait_quantums.insert({quantum, std::queue<int>()});
                        mm.memory[key].wait_quantums[quantum].push(status.MPI_SOURCE);
                    }
                }
                break;
            case SET_INFO:
                if (mm.is_read_only_mode) {
                    mm.memory[key].owners[quantum].push_back(status.MPI_SOURCE);
                } else {  // read_write mode
                    mm.memory[key].quantum_owner[quantum].first = true;
                    if (mm.memory[key].wait_quantums.find(quantum) != mm.memory[key].wait_quantums.end()) {
                        std::queue<int>& wait_queue = mm.memory[key].wait_quantums[quantum];
                        int source_rank = wait_queue.front();
                        wait_queue.pop();
                        if (wait_queue.size() == 0)
                            mm.memory[key].wait_quantums.erase(quantum);
                        int to_rank = mm.memory[key].quantum_owner[quantum].second;
                        mm.memory[key].quantum_owner[quantum] = {false, source_rank};
                        int to_request[4] = {GET_DATA_RW, key, quantum, source_rank};
                        MPI_Send(&to_rank, 1, MPI_INT, source_rank, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD);
                        MPI_Send(to_request, 4, MPI_INT, to_rank, SEND_DATA_TO_HELPER, MPI_COMM_WORLD);
                    }
                }
                break;
            case CHANGE_MODE:
                mm.num_of_change_mode_procs++;
                if (mm.num_of_change_mode_procs == mm.worker_size) {
                    mm.is_read_only_mode = request[1];
                    int ready = 1;
                    for (int i = 1; i < size; i++)
                        MPI_Send(&ready, 1, MPI_INT, i, GET_PERMISSION_FOR_CHANGE_MODE, MPI_COMM_WORLD);
                    mm.num_of_change_mode_procs = 0;
                    mm.num_of_change_mode++;
                }
                break;
        }
    }
}

void memory_manager::set_lock_read(int key, int quantum_index) {
    int request[] = {LOCK_READ, key, quantum_index};
    MPI_Send(request, 3, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);
    int ans;
    MPI_Status status;
    MPI_Recv(&ans, 1, MPI_INT, 0, GET_DATA_FROM_MASTER_HELPER_LOCK, MPI_COMM_WORLD, &status);
}

void memory_manager::set_lock_write(int key, int quantum_index) {
    // int request[] = {LOCK_WRITE, key, quantum_index};
    // MPI_Send(request, 3, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);
    // int ans;
    // MPI_Status status;
    // MPI_Recv(&ans, 1, MPI_INT, 0, GET_DATA_FROM_MASTER_HELPER, MPI_COMM_WORLD, &status);
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
        is_read_only_mode = true;
    } else if (mode == READ_WRITE) {
        int request[3] = {CHANGE_MODE, 0, -1};
        MPI_Send(request, 3, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);
        is_read_only_mode = false;
    }
    int is_ready;
    MPI_Status status;
    MPI_Recv(&is_ready, 1, MPI_INT, 0, GET_PERMISSION_FOR_CHANGE_MODE, MPI_COMM_WORLD, &status);
    mm.num_of_change_mode++;
}

void memory_manager::finalize() {
    MPI_Barrier(MPI_COMM_WORLD);
    if(rank == 0) {
        int request[4] = {-1, -1, -1, -1};
        MPI_Send(request, 3, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);
        for(int i = 1; i < size; i++) {
            MPI_Send(request, 4, MPI_INT, i, SEND_DATA_TO_HELPER, MPI_COMM_WORLD);
        }
    }
    assert(helper_thr.joinable());
    helper_thr.join();
    MPI_Finalize();
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