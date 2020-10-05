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
    memory_manager::memory_manager_init(argc, argv, error_helper_string);
    int n = atoi(argv[1]);
    parallel_vector pv(n);
    int rank, size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if(rank != 0) {
        for(int i = 0; i < n; i++) {
            pv.set_elem(i, i);
        }
        pv.change_mode(0, pv.get_num_quantums(), READ_ONLY);
        double t1 = MPI_Wtime();
        int sum = 0;
        for(int i = 0; i < n; i++) {
            int tmp = pv.get_elem(i);
            sum += pv.get_elem(i);
        }
        double t2 = MPI_Wtime();
        cout<<t2-t1<<" "<<sum<<" "<<rank<<"\n"<<std::flush;
        pv.change_mode(0, pv.get_num_quantums(), READ_WRITE);
        for(int i = 0; i < n; i++) {
            pv.set_elem(i, i+1);
        }
        for (int i = 0; i < pv.get_num_quantums(); i++)
            pv.change_mode(i, READ_ONLY);
        t1 = MPI_Wtime();
        sum = 0;
        for(int i = 0; i < n; i++) {
            int tmp = pv.get_elem(i);
            sum += pv.get_elem(i);
        }
        t2 = MPI_Wtime();
        cout<<t2-t1<<" "<<sum<<" "<<rank<<"\n";
        pv.change_mode(0, pv.get_num_quantums(), READ_WRITE);
        for(int i = 0; i < n; i++) {
            pv.set_elem(i, i);
        }
        pv.change_mode(0, pv.get_num_quantums(), READ_ONLY);
        t1 = MPI_Wtime();
        sum = 0;
        for(int i = 0; i < n; i++) {
            int tmp = pv.get_elem(i);
            sum += pv.get_elem(i);
        }
        t2 = MPI_Wtime();
        cout<<t2-t1<<" "<<sum<<" "<<rank<<"\n"<<std::flush;
        pv.change_mode(0, pv.get_num_quantums(), READ_WRITE);
        for(int i = 0; i < n; i++) {
            pv.set_elem(i, i+1);
        }
        pv.change_mode(0, pv.get_num_quantums(), READ_ONLY);
        t1 = MPI_Wtime();
        sum = 0;
        for(int i = 0; i < n; i++) {
            int tmp = pv.get_elem(i);
            sum += pv.get_elem(i);
        }
        t2 = MPI_Wtime();
        cout<<t2-t1<<" "<<sum<<" "<<rank<<"\n";
    }
    memory_manager::finalize();
    return 0;
}
