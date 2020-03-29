#include <thread>
#include <mpi.h>
#include <vector>
#include <iostream>
#include "memory_manager.h"
// #include "parallel_vector.h"

// посылка: [операция, номер структуры, откуда требуются данные; требуемый номер элемента, кому требуется переслать объект]
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
    // создание своего типа для пересылки посылок ???
}

int memory_manager::create_object(int number_of_elements) {
    memory_line line;
    line.logical_size = number_of_elements;
    if(rank == 0) {
        line.quantums.resize((number_of_elements + QUANTUM_SIZE - 1) / QUANTUM_SIZE);
        for(int i = 0; i < line.quantums.size(); i++)
            line.quantums[i] = -1;
    } else {
        int portion = number_of_elements/worker_size + (worker_rank < number_of_elements%worker_size?1:0);
        line.vector.resize(portion);
    }
    memory.push_back(line);
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

int memory_manager::get_size_of_portion(int key) {
    return memory[key].vector.size();
}


int memory_manager::get_data(int key, int index_of_element) {
    // send to 0, чтобы узнать, какой процесс затребовал элемент?
    std::pair<int, int> index = get_number_of_process_and_index(key, index_of_element);
    if(rank == index.first)
        return memory[key].vector[index.second];
    int request[] = {GET_DATA, key, index.second, -1};
    MPI_Send(request, 4, MPI_INT, index.first, SEND_DATA_TO_HELPER, MPI_COMM_WORLD);
    int data;
    MPI_Status status;
    MPI_Recv(&data, 1, MPI_INT, index.first, GET_DATA_FROM_HELPER, MPI_COMM_WORLD, &status);
    return data;
}

void memory_manager::set_data(int key, int index_of_element, int value) {
    std::pair<int, int> index = get_number_of_process_and_index(key, index_of_element);
    if(rank == index.first)
        memory[key].vector[index.second] = value;
    int request[] = {SET_DATA, key, index.second, value};
    MPI_Send(request, 4, MPI_INT, index.first, SEND_DATA_TO_HELPER, MPI_COMM_WORLD);
    int tmp;
    MPI_Status status;
    MPI_Recv(&tmp, 1, MPI_INT, index.first, GET_DATA_FROM_HELPER, MPI_COMM_WORLD, &status);
}

void memory_manager::copy_data(int key_from, int key_to) {
    memory[key_to] = memory[key_from];
}

int memory_manager::get_data_by_index_on_process(int key, int index) {
    return memory[key].vector[index];
}

void memory_manager::set_data_by_index_on_process(int key, int index, int value) {
    memory[key].vector[index] = value;
}

std::pair<int, int> memory_manager::get_number_of_process_and_index(int key, int index) {
    int number_proc, number_elem;
    int tmp1 = int(memory[key].logical_size)%worker_size;
    int tmp2 = int(memory[key].logical_size)/worker_size;
    if(index < tmp1*(tmp2+1)) {
        number_proc = index/(tmp2+1);
        number_elem = index%(tmp2+1);
    } else {
        int tmp = index - tmp1*(tmp2+1);
        number_proc = tmp1 + tmp/tmp2;
        number_elem = tmp%tmp2;
    }
    number_proc += 1;
    return {number_proc, number_elem};
}

int memory_manager::get_logical_index_of_element(int key, int index, int process) {

    int number_elem;
    process -= 1;
    if(process < memory[key].logical_size%int(memory[key].vector.size())) {
        number_elem = process*int(memory[key].vector.size()) + index;
    }
    else {
        number_elem = (memory[key].logical_size%int(memory[key].vector.size()))*(int(memory[key].vector.size())+1)
                                + (process-memory[key].logical_size%int(memory[key].vector.size()))*int(memory[key].vector.size()) + index;
    }
    return number_elem;
}

void worker_helper_thread() {
    int request[4];
    MPI_Status status;
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    while(true) {
        MPI_Recv(request, 4, MPI_INT, MPI_ANY_SOURCE, SEND_DATA_TO_HELPER, MPI_COMM_WORLD, &status);
        if(request[0] == -1) {
            break;
        }
        if(request[0] == GET_DATA) {
            int to_rank = status.MPI_SOURCE;
            int data = mm.memory[request[1]].vector[request[2]];
            MPI_Send(&data, 1, MPI_INT, to_rank, GET_DATA_FROM_HELPER, MPI_COMM_WORLD); // MPI_ISend?
        }
        else if(request[0] == SET_DATA) {
            mm.memory[request[1]].vector[request[2]] = request[3];
            int to_rank = status.MPI_SOURCE;
            int tmp = 1;
            MPI_Send(&tmp, 1, MPI_INT, to_rank, GET_DATA_FROM_HELPER, MPI_COMM_WORLD);
        }
    }
}

void master_helper_thread() {
    int request[3];
    MPI_Status status;
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    while(true) {
        MPI_Recv(&request, 3, MPI_INT, MPI_ANY_SOURCE, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD, &status);
        if(request[0] == -1) {
            break;
        }
        int key = request[1], quantum = request[2];
        if(request[0] == LOCK) {
            if(mm.memory[key].quantums[quantum] == -1) {
                int to_rank = status.MPI_SOURCE;
                int tmp = 1;
                mm.memory[key].quantums[quantum] = status.MPI_SOURCE;
                MPI_Send(&tmp, 1, MPI_INT, to_rank, GET_DATA_FROM_MASTER_HELPER, MPI_COMM_WORLD);
            } else {
                if(mm.memory[key].wait.find(quantum) == mm.memory[key].wait.end())
                    mm.memory[key].wait.insert({quantum, std::queue<int>{}});
                mm.memory[key].wait[quantum].push(status.MPI_SOURCE);
            }
        }
        else if(request[0] == UNLOCK) {
            if(mm.memory[key].quantums[quantum] == status.MPI_SOURCE) {
                mm.memory[key].quantums[quantum] = -1;
                if(mm.memory[key].wait.find(quantum) != mm.memory[key].wait.end()) {
                    int to_rank = mm.memory[key].wait[quantum].front();
                    mm.memory[key].wait[quantum].pop();
                    mm.memory[key].quantums[quantum] = to_rank;
                    if(mm.memory[key].wait[quantum].size() == 0)
                        mm.memory[key].wait.erase(quantum);
                    int tmp = 1;
                    MPI_Send(&tmp, 1, MPI_INT, to_rank, GET_DATA_FROM_MASTER_HELPER, MPI_COMM_WORLD);
                }
            }
        }
    }
}

void memory_manager::set_lock(int key, int quantum_index) {
    int request[] = {LOCK, key, quantum_index};
    MPI_Send(request, 3, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);
    int ans;
    MPI_Status status;
    MPI_Recv(&ans, 1, MPI_INT, 0, GET_DATA_FROM_MASTER_HELPER, MPI_COMM_WORLD, &status);
}

void memory_manager::unset_lock(int key, int quantum_index) {
    int request[3] = {UNLOCK, key, quantum_index};
    MPI_Send(request, 3, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);
}

void memory_manager::finalize() {
    MPI_Barrier(MPI_COMM_WORLD);
    int request[4] = {-1, -1, -1, -1};
    if(rank == 0) {
        MPI_Send(request, 3, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);
        for(int i = 1; i < size; i++) {
            MPI_Send(request, 4, MPI_INT, i, SEND_DATA_TO_HELPER, MPI_COMM_WORLD);
        }
    }
    if(helper_thr.joinable())   
        helper_thr.join();
    MPI_Finalize();
}

memory_manager::~memory_manager() {
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
}