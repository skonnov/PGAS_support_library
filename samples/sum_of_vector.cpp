#include <iostream>
#include <cassert>
#include <mpi.h>
#include "parallel_vector.h"
// #include "parallel_for.h"
#include "memory_manager.h"

using namespace std;

extern memory_manager mm;

int main(int argc, char** argv) {
    // std::cout<<" BEGIN \n"<<std::flush;
    mm.memory_manager_init(argc, argv);
    // std::cout<<" AFTER INIT \n"<<std::flush;
    // freopen("log.txt", "w", stdout);
    assert(argc >= 1);
    int n = atoi(argv[1]);
    parallel_vector pv(n);
    int rank, size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if(rank != 0) {
        // std::cout<<"("<<rank<<" | 1)\n"<<std::flush;
        // if(rank == 1)
            for(int i = 0; i < n; i++) {
                pv.set_elem(i, i);
            }
        // std::cout<<"("<<rank<<" | 2)\n"<<std::flush;
        mm.change_mode(READ_ONLY);
        double t1 = MPI_Wtime();
        int sum = 0;
        for(int i = 0; i < n; i++) {
            int tmp = pv.get_elem(i);
            sum += pv.get_elem(i);
        }
        double t2 = MPI_Wtime();
        cout<<t2-t1<<" "<<sum<<" "<<rank<<"\n"<<std::flush;
        mm.change_mode(READ_WRITE);
        for(int i = 0; i < n; i++) {
            pv.set_elem(i, i+1);
        }
        mm.change_mode(READ_ONLY);
        t1 = MPI_Wtime();
        sum = 0;
        for(int i = 0; i < n; i++) {
            int tmp = pv.get_elem(i);
            sum += pv.get_elem(i);
        }
        t2 = MPI_Wtime();
        cout<<t2-t1<<" "<<sum<<" "<<rank<<"\n";
        mm.change_mode(READ_WRITE);
        // std::cout<<"("<<rank<<" | 1)\n"<<std::flush;
        // if(rank == 1)
            for(int i = 0; i < n; i++) {
                pv.set_elem(i, i);
            }
        // std::cout<<"("<<rank<<" | 2)\n"<<std::flush;
        mm.change_mode(READ_ONLY);
        t1 = MPI_Wtime();
        sum = 0;
        for(int i = 0; i < n; i++) {
            int tmp = pv.get_elem(i);
            sum += pv.get_elem(i);
        }
        t2 = MPI_Wtime();
        cout<<t2-t1<<" "<<sum<<" "<<rank<<"\n"<<std::flush;
        mm.change_mode(READ_WRITE);
        for(int i = 0; i < n; i++) {
            pv.set_elem(i, i+1);
        }
        mm.change_mode(READ_ONLY);
        t1 = MPI_Wtime();
        sum = 0;
        for(int i = 0; i < n; i++) {
            int tmp = pv.get_elem(i);
            sum += pv.get_elem(i);
        }
        t2 = MPI_Wtime();
        cout<<t2-t1<<" "<<sum<<" "<<rank<<"\n";
    }
    // std::cout<<"("<<rank<<" | 3)\n"<<std::flush;
    mm.finalize();
    // std::cout<<rank<<" end of main\n"<<std::flush;
    return 0;
}