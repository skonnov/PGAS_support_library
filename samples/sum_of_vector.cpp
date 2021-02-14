#include <iostream>
#include <cassert>
#include <string>
#include <mpi.h>
#include <fstream>
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
    parallel_vector pv(n, 1), pv2(n, 1);
    int rank, size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if(rank != 0) {
        for (int j = 0; j < 100; j++) {
            for(int i = 0; i < n; i++) {
                pv.set_elem(i, i);
            }
            std::cout<<"----------------------"<<j<<"----------------------"<<std::endl;
        }
    }
    memory_manager::finalize();
    return 0;
    if(rank != 0) {
        for(int i = 0; i < n; i++) {
            pv.set_elem(i, i);
        }
        pv.change_mode(0, pv.get_num_quantums(), READ_ONLY);
        pv.print("test.txt");
        // pv2.read("test.txt", n);
        double t1 = MPI_Wtime();
        if (rank == 1) {
            pv2.read("test.txt", n, 0, n);
            for(int i = 0; i < n; i++)
                std::cout<<pv2.get_elem(i)<<" ";
            std::cout<<std::endl;
        }
        int sum = 0;
        for(int i = 0; i < n; i++) {
            int tmp = pv.get_elem(i);
            sum += pv.get_elem(i);
        }
        double t2 = MPI_Wtime();
        pv.print("test.txt");
        cout<<t2-t1<<" "<<sum<<" "<<rank<<std::endl;
        pv.change_mode(0, pv.get_num_quantums(), READ_WRITE);
        if(rank == 1)
            std::cout<<"------ 1"<<std::endl;
        for(int i = 0; i < n; i++) {
            pv.set_elem(i, i+1);
        }
        std::cout << "-- before change_mode "<<rank<<std::endl;
        pv.change_mode(0, pv.get_num_quantums(), READ_ONLY);
        std::cout << "-- after change_mode "<<rank<<std::endl;
        t1 = MPI_Wtime();
        sum = 0;
        for(int i = 0; i < n; i++) {
            int tmp = pv.get_elem(i);
            sum += pv.get_elem(i);
        }
        t2 = MPI_Wtime();
        cout<<t2-t1<<" "<<sum<<" "<<rank<<std::endl;
        pv.change_mode(0, pv.get_num_quantums(), READ_WRITE);
        if(rank == 1)
            std::cout<<"------ 2"<<std::endl;
        for(int i = 0; i < n; i++) {
            pv.set_elem(i, i);
        }
        std::cout << "-- before change_mode "<<rank<<std::endl;
        pv.change_mode(0, pv.get_num_quantums(), READ_ONLY);
        std::cout << "-- after change_mode "<<rank<<std::endl;
        t1 = MPI_Wtime();
        sum = 0;
        for(int i = 0; i < n; i++) {
            int tmp = pv.get_elem(i);
            sum += pv.get_elem(i);
        }
        t2 = MPI_Wtime();
        cout<<t2-t1<<" "<<sum<<" "<<rank<<std::endl;
        pv.change_mode(0, pv.get_num_quantums(), READ_WRITE);
        if(rank == 1)
            std::cout<<"------ 3"<<std::endl;
        for(int i = 0; i < n; i++) {
            pv.set_elem(i, i+1);
        }
        std::cout << "-- before change_mode "<<rank<<std::endl;
        pv.change_mode(0, pv.get_num_quantums(), READ_ONLY);
        std::cout << "-- after change_mode "<<rank<<std::endl;
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
