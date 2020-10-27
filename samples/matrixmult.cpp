#include <iostream>
#include <cmath>
#include "memory_manager.h"
#include "parallel_vector.h"

std::pair<int, int> get_grid_rank(int MPI_rank, int q) {
    return { (MPI_rank-1)/q, (MPI_rank-1)%q };
}

void matrix_mult(parallel_vector* pv1, parallel_vector* pv2, parallel_vector * pv3, int i1, int j1, int i2, int j2, int i3, int j3, int n, int num_in_block) {
    for(int i = 0; i < num_in_block; i++) {
        for(int j = 0; j < num_in_block; j++) {
            int i3_teq = i3 + i;
            int j3_teq = j3 + j;
            int temp = pv3->get_elem(i3_teq*n+j3_teq);
            for(int k = 0; k < num_in_block; k++) {
                // std::cout<<"("<<i3_teq<<" "<<j3_teq<<") ("<<(i1+i)<<" "<<j1+k<<") ("<<(i2+j)<<" "<<j2+k<<")\n";
                temp += pv1->get_elem((i1+i)*n+j1+k)*pv2->get_elem((i2+j)*n+j2+k);
            }
            pv3->set_elem(i3_teq*n+j3_teq, temp);
        }
    }
}

void print(parallel_vector* pv1, parallel_vector* pv2, parallel_vector * pv3, int n) {
    for (int i = 0; i < n*n; i++) {
        if(i && i%n == 0)
            std::cout<<std::endl;
        std::cout<<pv1->get_elem(i)<<" ";
    }
    std::cout<<std::endl<<std::endl;
    for (int i = 0; i < n*n; i++) {
        if(i && i%n == 0)
            std::cout<<std::endl;
        std::cout<<pv2->get_elem(i)<<" ";
    }
    std::cout<<std::endl<<std::endl;

    for (int i = 0; i < n*n; i++) {
        if(i && i%n == 0)
            std::cout<<std::endl;
        std::cout<<pv3->get_elem(i)<<" ";
    }
    std::cout<<std::endl;
}

int main(int argc, char** argv) {
    if (argc <= 1) {
        return 1;
    }
    memory_manager::memory_manager_init(argc, argv);
    int size_workers = memory_manager::get_MPI_size()-1;
    int q = static_cast<int>(sqrt(static_cast<double>(size_workers)));
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
                pv2.set_elem(i*n+j, j*n+i);
            }
        }
        pv1.change_mode(0, n*n/QUANTUM_SIZE, READ_ONLY);
        pv2.change_mode(0, n*n/QUANTUM_SIZE, READ_ONLY);
        for (int iter = 0; iter < q; iter++) {
            int i_begin = grid_ind.first * num_in_block;
            int j_begin = ((grid_ind.first + iter)%q)* num_in_block;
            int i2_begin = grid_ind.second * num_in_block;
            int j2_begin = ((grid_ind.first + iter)%q) * num_in_block;
            int i3_begin = grid_ind.first * num_in_block;
            int j3_begin = grid_ind.second * num_in_block;
            matrix_mult(&pv1, &pv2, &pv3, i_begin, j_begin, i2_begin, j2_begin, i3_begin, j3_begin, n, num_in_block);
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if(rank == 1)
        print(&pv1, &pv2, &pv3, n);
    memory_manager::finalize();
    return 0;
}