#include <iostream>
#include <cassert>
#include <string>
#include <mpi.h>
#include "parallel_vector.h"
#include "memory_manager.h"

using namespace std;

int main(int argc, char** argv) {
    std::string error_helper_string = "mpiexec -n <numproc> "+std::string(argv[0])+" <length>";
    if (argc <= 1) {
        std::cout << "Error: you need to pass length of vector!" << std::endl;
        std::cout << "Usage:\n" << error_helper_string << std::endl;
        return 1;
    }
    memory_manager::init(argc, argv, error_helper_string);
    int n = atoi(argv[1]);
    parallel_vector<int> pv(n);
    int rank, size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if(rank != 0) {
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
    memory_manager::finalize();
    return 0;
}
