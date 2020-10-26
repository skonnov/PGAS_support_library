#include "memory_manager.h"
#include "parallel_vector.h"

std::pair<int, int> get_grid_rank(int MPI_rank, int q) {
    return { (MPI_rank-1)/q, (MPI_rank-1)%q };
}


int get_MPI_rank(int str, int col, int q) {
    return str*q+col+1;
}

int main(int argc, char** argv) {
    if (argc <= 1) {
        return 1;
    }
    memory_manager::memory_manager_init(argc, argv);
    int size_workers = memory_manager::get_MPI_size()-1;
    int q = sqrt(double(size_workers));
    if (q*q != size_workers) {
        std::cout<<q<<std::endl;
        memory_manager::finalize();
        return 2;
    }
    int n = atoi(argv[1]);
    int num_in_block = n/q;
    if (n%q) {
        memory_manager::finalize();
        return 3;
    }
    parallel_vector pv1(n*n), pv2(n*n), pv3(n*n);
    int rank = memory_manager::get_MPI_rank();
    if (rank != 0) {
        // init
        std::pair<int, int> grid_ind = get_grid_rank(rank, q);
        int i_begin = grid_ind.first * num_in_block;
        int j_begin = grid_ind.second * num_in_block;
        for (int i = i_begin; i < i_begin + num_in_block; i++) {
            for (int j = j_begin; j < j_begin + num_in_block; j++) {
                pv1.set_elem(i*n+j, i*n+j);
                pv3.set_elem(i*n+j, 0);
            }
        }
        for (int i = j_begin; i < j_begin + num_in_block; i++) {
            for (int j = i_begin; j < i_begin + num_in_block; j++) {
                std::cout<<rank<<" "<<i<<" "<<j<<std::endl;
                pv2.set_elem(i*n+j, i*n+j);
            }
        }
        pv1.change_mode(0, n*n/QUANTUM_SIZE, READ_ONLY);
        pv2.change_mode(0, n*n/QUANTUM_SIZE, READ_ONLY);
    }
    // MPI_Barrier(MPI_COMM_WORLD);
    // if (rank != 0) {
    //     if (rank == 1)
    //     {
    //         for (int i = 0; i < n*n; i++) {
    //             if(i && i%n == 0)
    //                 std::cout<<std::endl;
    //         }
    //         std::cout<<std::endl<<std::endl;
    //         for (int i = 0; i < n*n; i++) {
    //             if(i && i%n == 0)
    //                 std::cout<<std::endl;
    //         }
    //         std::cout<<std::endl;
    //     }
    // }
    memory_manager::finalize();
    return 0;
}