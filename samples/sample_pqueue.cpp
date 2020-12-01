#include <iostream>
#include <cmath>
#include <algorithm>
#include "memory_manager.h"
#include "parallel_priority_queue.h"

int main(int argc, char** argv) {
    int num_of_quantums_proc, quantum_size;
    num_of_quantums_proc = atoi(argv[1]);
    quantum_size = atoi(argv[2]);
    
    if(argc < 3) {
        std::cout<<"need args!"<<std::endl;
        return 1;
    }

    memory_manager::memory_manager_init(argc, argv);
    parallel_priority_queue ppq(num_of_quantums_proc, quantum_size);
    int rank = memory_manager::get_MPI_rank();
    int size = memory_manager::get_MPI_size();
    if (rank != 0) {
        for (int i = 0; i < num_of_quantums_proc*quantum_size*(size-1); i++) {
            ppq.insert(i);
        }
    }
    if(rank == 1)
        std::cout<<ppq.get_max() << std::endl;
    return 0;
}