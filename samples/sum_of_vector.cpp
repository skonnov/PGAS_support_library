#include <iostream>
#include <cassert>
#include <mpi.h>
#include "parallel_vector.h"
// #include "parallel_for.h"
#include "memory_manager.h"

using namespace std;

int main(int argc, char** argv) {
    memory_manager::memory_manager_init(argc, argv);
    assert(argc >= 1);
    int n = atoi(argv[1]);
    parallel_vector pv(n);
    int rank, size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if(rank != 0) {
        for(int i = 0; i < n; i++) {
            pv.set_elem(i, i);
        }
        memory_manager::change_mode(READ_ONLY);
        double t1 = MPI_Wtime();
        int sum = 0;
        for(int i = 0; i < n; i++) {
            int tmp = pv.get_elem(i);
            sum += pv.get_elem(i);
        }
        double t2 = MPI_Wtime();
        cout<<t2-t1<<" "<<sum<<" "<<rank<<"\n"<<std::flush;
        memory_manager::change_mode(READ_WRITE);
        for(int i = 0; i < n; i++) {
            pv.set_elem(i, i+1);
        }
        memory_manager::change_mode(READ_ONLY);
        t1 = MPI_Wtime();
        sum = 0;
        for(int i = 0; i < n; i++) {
            int tmp = pv.get_elem(i);
            sum += pv.get_elem(i);
        }
        t2 = MPI_Wtime();
        cout<<t2-t1<<" "<<sum<<" "<<rank<<"\n";
        memory_manager::change_mode(READ_WRITE);
        for(int i = 0; i < n; i++) {
            pv.set_elem(i, i);
        }
        memory_manager::change_mode(READ_ONLY);
        t1 = MPI_Wtime();
        sum = 0;
        for(int i = 0; i < n; i++) {
            int tmp = pv.get_elem(i);
            sum += pv.get_elem(i);
        }
        t2 = MPI_Wtime();
        cout<<t2-t1<<" "<<sum<<" "<<rank<<"\n"<<std::flush;
        memory_manager::change_mode(READ_WRITE);
        for(int i = 0; i < n; i++) {
            pv.set_elem(i, i+1);
        }
        memory_manager::change_mode(READ_ONLY);
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
