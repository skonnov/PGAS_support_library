#include <iostream>
#include "memory_manager.h"
#include "parallel_vector.h"


// #define MAX_TASK 5

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
    return 0;
}

static void show_usage() {
    if (memory_manager::get_MPI_rank() == 1)
        std::cerr << "Usage: mpiexec <-n matrices size> matrixmult_queue <-d num_of_divisions> [-s seed]"<<std::endl;
}

template<class T>
void matrix_mult(parallel_vector<T>* pv1, parallel_vector<T>* pv2, parallel_vector<T> * pv3, int i1, int j1, int i2, int j2, int i3, int j3, int n, int num_in_block) {
    for (int i = 0; i < num_in_block; i++) {
        for (int j = 0; j < num_in_block; j++) {
            int i3_teq = i3 + i;
            int j3_teq = j3 + j;
            int temp = pv3->get_elem(i3_teq*n+j3_teq);
            for (int k = 0; k < num_in_block; k++) {
                temp += pv1->get_elem((i1+i)*n+j1+k)*pv2->get_elem((i2+j)*n+j2+k);
            }
            pv3->set_elem(i3_teq*n+j3_teq, temp);
        }
    }
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
    // read pva, pvb, pvc
    pva.change_mode(0, pva.get_num_quantums(), READ_ONLY);
    pvb.change_mode(0, pvb.get_num_quantums(), READ_ONLY);
    int part_size = n / div_num;
    std::queue<std::pair<int, int>> qu;
    int rank = memory_manager::get_MPI_rank();
    if (rank != 0) {
        if (rank == 1) {
            for (int i = 0; i < div_num; ++i) {
                for (int j = 0; j < div_num; ++j) {
                    qu.push({i, j});
                }
            }
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);

    if (rank != 0) {
        if (rank == 1) {
            while (!qu.empty()) {
                // for (int i = 0; i < MAX_TASK && !qu.empty(); ++i) {
                    // std::pair<int, int> p = qu.front();
                    // send to other procs
                    qu.pop();
                // }
            }
        } else {
            int end_flag = 0;
            // while(!end_flag) {

            // }
        }
    }

    memory_manager::finalize();
    return 0;
}
