#include <iostream>
#include <cassert>
#include <mpi.h>
#include "parallel_vector.h"
// #include "parallel_for.h"
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
    for(int i = 0; i < n; i++) {
        pv.set_elem(i, i);
    }
    double t1 = MPI_Wtime();
    int sum = 0;
    if(rank == 0) {
        for(int i = 0; i < n; i++) {
            int tmp = pv.get_elem(i);
            sum += pv.get_elem(i);
        }
    }
    double t2 = MPI_Wtime();
    if(rank == 0)
        cout<<t2-t1;
    mm.finalize();
    std::cout<<"end of main\n";
    return 0;
}