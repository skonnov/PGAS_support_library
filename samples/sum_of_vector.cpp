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
    memory_manager::init(argc, argv, error_helper_string);
    int n = atoi(argv[1]);
    parallel_vector<int> pv(n, 1), pv2(n);
    int rank, size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank != 0) {
        for (int i = 0; i < n; ++i) {
            pv.set_elem(i, i);
        }
        pv.change_mode(0, pv.get_num_quantums(), READ_ONLY);
        pv.print("test.txt");
        // pv2.read("test.txt", n);
        double t1 = MPI_Wtime();
        if (rank == 1) {
            pv2.read("test.txt", n, 0, n);
            for (int i = 0; i < n; ++i)
                std::cout << pv2.get_elem(i) << " ";
            std::cout << std::endl;
        }
        int sum = 0;
        for (int i = 0; i < n; ++i) {
            int tmp = pv.get_elem(i);
            sum += pv.get_elem(i);
        }
        double t2 = MPI_Wtime();
        pv.print("test.txt");
        cout << t2-t1 << " " << sum << " " << rank << std::endl;
        pv.change_mode(0, pv.get_num_quantums(), READ_WRITE);
        if (rank == 1)
            std::cout << "------" << std::endl;
        for (int i = 0; i < n; ++i) {
            pv.set_elem(i, i+1);
        }
        pv.change_mode(0, pv.get_num_quantums(), READ_ONLY);
        t1 = MPI_Wtime();
        sum = 0;
        for (int i = 0; i < n; ++i) {
            int tmp = pv.get_elem(i);
            sum += pv.get_elem(i);
        }
        t2 = MPI_Wtime();
        cout << t2-t1 << " " << sum << " " << rank << std::endl;
        pv.change_mode(0, pv.get_num_quantums(), READ_WRITE);
        if (rank == 1)
            std::cout << "------" << std::endl;
        for (int i = 0; i < n; ++i) {
            pv.set_elem(i, i);
        }
        pv.change_mode(0, pv.get_num_quantums(), READ_ONLY);
        t1 = MPI_Wtime();
        sum = 0;
        for (int i = 0; i < n; ++i) {
            int tmp = pv.get_elem(i);
            sum += pv.get_elem(i);
        }
        t2 = MPI_Wtime();
        cout << t2-t1 << " " << sum << " " << rank << std::endl;
        pv.change_mode(0, pv.get_num_quantums(), READ_WRITE);
        if (rank == 1)
            std::cout << "------" << std::endl;
        for (int i = 0; i < n; ++i) {
            pv.set_elem(i, i+1);
        }
        pv.change_mode(0, pv.get_num_quantums(), READ_ONLY);
        t1 = MPI_Wtime();
        sum = 0;
        for (int i = 0; i < n; ++i) {
            int tmp = pv.get_elem(i);
            sum += pv.get_elem(i);
        }
        t2 = MPI_Wtime();
        cout << t2-t1 << " " << sum << " " << rank << "\n";
    }
    if (rank == 1) {
        memory_manager::wait();
        std::cout << "AFTER WAIT!" << std::endl;
    } else if (rank == 2) {
        std::cout << "NOTIFY!" << std::endl;
        memory_manager::notify(1);
    }
    memory_manager::finalize();
    return 0;
}
