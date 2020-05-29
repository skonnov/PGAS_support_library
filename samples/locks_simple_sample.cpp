#include <iostream>
#include <cassert>
#include <mpi.h>
#include "parallel_vector.h"
#include "memory_manager.h"

using namespace std;

extern memory_manager mm;

int main(int argc, char** argv) {
    mm.memory_manager_init(argc, argv);
    assert(argc >= 1);
    int n = atoi(argv[1]);
    parallel_vector pv(n);
    int rank, size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if(rank == 1) {
        for (int i = 0; i < n; i++) {
            pv.set_elem(i, 0);
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if (rank != 0) {
        for(int i = 0; i < n; i++)
        {
            pv.set_lock(pv.get_quantum(i));
            pv.set_elem(i, pv.get_elem(i) + 1);
            pv.unset_lock(pv.get_quantum(i));
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if(rank == 1) {
        for(int i = 0; i < n; i++)
            std::cout<<pv.get_elem(i)<<" ";
        std::cout<<"\n";
    }
    mm.finalize();
    return 0;
}