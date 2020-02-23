#include <thread>
#include <mpi.h>
#include <vector>
#include <iostream>
#include "memory_manager.h"
// #include "parallel_vector.h"

#define GET_DATA_FROM_HELPER 123
#define SEND_DATA_TO_HELPER 234
// посылка: [номер структуры, откуда требуются данные; требуемый номер элемента, кому требуется переслать объект]
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
    std::cout<<rank<<" "<<size<<"\n";
    std::cout<<"this is mm init\n";
    helper_thr = std::thread(helper_thread);
    // создание своего типа для пересылки посылок ???
}

// list of free vectors
int memory_manager::create_object(int number_of_elements) {  // int*???
    int portion = number_of_elements/size + (number_of_elements%size < rank?1:0);
    memory.push_back(std::vector<int>(number_of_elements));
    return memory.size()-1;
    return 0;
    // разделение памяти по процессам?
}

// void memory_manager::delete_object(int* object) {  // int*???
//     auto tmp = map_pointer_to_int.find(object);
//     if(tmp == map_pointer_to_int.end())
//         throw -1;
//     auto tmp2 = map_int_to_pointer.find(tmp->second);  // ???
//     if(tmp2 == map_int_to_pointer.end())
//         throw -2;
//     map_int_to_pointer.erase(tmp2);
//     map_pointer_to_int.erase(tmp);
// }


int memory_manager::get_data(int key, int index_of_element) {
    // send to 0, чтобы узнать, какой процесс затребовал элемент?
    std::pair<int, int> index = get_number_of_process_and_index(key, index_of_element);
    if(rank == index.first)
        return memory[key][index.second];
    int request[] = {key, index.second};
    MPI_Send(&request, 2, MPI_INT, index.first, SEND_DATA_TO_HELPER, MPI_COMM_WORLD);
    int data;
    MPI_Status status;
    MPI_Recv(&data, 1, MPI_INT, index.first, GET_DATA_FROM_HELPER, MPI_COMM_WORLD, &status);
    return data;
}

void memory_manager::set_data(int key, int index_of_element, int value) {
    std::pair<int, int> index = get_number_of_process_and_index(key, index_of_element);
    if(rank == index.first)
        memory[key][index.second] = value;
}

std::pair<int, int> memory_manager::get_number_of_process_and_index(int key, int index) {
    int number_proc;
    if(index < (int(memory[key].size())%size)*(int(memory[key].size())/size+1)) {
        number_proc = index/(memory[key].size()/size+1);
    } else {
        int tmp = index - (int(memory[key].size())%size)*(int(memory[key].size())/size+1);
        number_proc = tmp/(int(memory[key].size())/size) + (int(memory[key].size())%size);
    }
    int number_elem;
    if(index < (int(memory[key].size())%size)*(int(memory[key].size())/size+1)) {
        number_elem = index%(memory[key].size()/size+1);
    } else {
        int tmp = index - (memory[key].size()%size)*(memory[key].size()/size+1);
        number_elem = tmp%(memory[key].size()/size);
    }
    return {number_proc, number_elem};
 }

void helper_thread() {
    int request[2];
    MPI_Status status;
    while(true) {
        std::cout<<"helper_thread hello! "<<mm.helper_thr.get_id()<<"\n";
        MPI_Recv(request, 2, MPI_INT, 0, SEND_DATA_TO_HELPER, MPI_COMM_WORLD, &status);
        std::cout<<mm.rank<<" Get request! "<<request[0]<<" "<<request[1]<<" "<<status.MPI_SOURCE<<"\n";
        if(request[0] == -1) {
            std::cout<<"helper_thread_end\n";
            break;
        }
        int to_rank = status.MPI_SOURCE;
        int data = mm.memory[request[0]][request[1]];
        MPI_Send(&data, 1, MPI_INT, to_rank, GET_DATA_FROM_HELPER, MPI_COMM_WORLD); // MPI_ISend?
    }
}

void memory_manager::finalize() {
    std::cout<<"this is finalize "<<helper_thr.get_id()<<" | rank = "<<rank<<"\n";
    int request[2] = {-1, -1};
    if(rank == 0) {
        for(int i = 0; i < size; i++) {
            std::cout<<"MPI_Send to rank "<<rank<<" from finalize, size = "<<size<<"\n";
            MPI_Send(request, 2, MPI_INT, i, SEND_DATA_TO_HELPER, MPI_COMM_WORLD);
        }
    }
    if(helper_thr.joinable())   
        helper_thr.join();
    std::cout<<"join ended\n";
    MPI_Finalize();
}

memory_manager::~memory_manager() {
    std::cout<<"this is destructor "<<helper_thr.get_id()<<" | rank = "<<rank<<"\n";
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