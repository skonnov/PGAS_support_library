#include <iostream>
#include "memory_manager.h"
#include "parallel_vector.h"

int get_args(int argc, char** argv, int& n, int& div_num, int& seed) {
    n = -1, div_num = -1, seed = 0;
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "-n") {
            if (i+1 < argc) {
                n = atoi(argv[++i]);
            } else {
                return -1;
            }
        }

        if (std::string(argv[i]) == "-d") {
            if (i+1 < argc) {
                div_num = atoi(argv[++i]);
            } else {
                return -1;
            }
        }

        if (std::string(argv[i]) == "-s") {
            if (i+1 < argc) {
                seed = atoi(argv[++i]);
            } else {
                return -1;
            }
        }
    }

    if (n == -1) {
        if (memory_manager::get_MPI_rank() == 1)
            std::cerr<<"You need to define matrixes size!"<<std::endl;
        return -1;
    }

    if (n < 0) {
        if (memory_manager::get_MPI_rank() == 1)
            std::cerr<<"matrices size must be positive!"<<std::endl;
        return -1;
    }

    if (div_num < 0) {
        if (memory_manager::get_MPI_rank() == 1)
            std::cerr<<"number of matrices divisions must be positive!"<<std::endl;
        return -1;
    }

    if (n % div_num != 0) {
        if (memory_manager::get_MPI_rank() == 1)
            std::cerr<<"matrices size must be divided by the number divisions!"<<std::endl;
        return -1;
    }
}

static void show_usage() {
    if (memory_manager::get_MPI_rank() == 1)
        std::cerr << "Usage: mpiexec <-n matrices size> matrixmult_queue <-d num_of_divisions> [-s seed]"<<std::endl;
}

int main(int argc, char** argv) {
    memory_manager::init(argc, argv);
    int n, div_num, seed;
    int res = get_args(argc, argv, n, div_num, seed);
    if (res == -1) {
        show_usage();
        memory_manager::finalize();
        return 0;
    }
    parallel_vector<int> pva(n * n), pvb(n * n), pvc (n * n);
    memory_manager::finalize();
    return 0;
}
