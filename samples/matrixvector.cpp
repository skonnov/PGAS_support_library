#include <iostream>
#include <mpi.h>
#include <cassert>
#include "memory_manager.h"
#include "parallel_vector.h"

int main(int argc, char** argv) { // b*a
    mm.memory_manager_init(argc, argv);
    double t1 = MPI_Wtime();
    assert(argc > 2);
    int n = atoi(argv[1]), m = atoi(argv[2]);
    int rank, size;
    rank = mm.get_MPI_rank();
    size = mm.get_MPI_size();
    parallel_vector pv(n*m);
    parallel_vector ans(m);
    std::vector<int> b(n);
    if (rank != 0) {
        int worker_rank = rank - 1;
        int worker_size = size - 1;
        int portion = m/worker_size + (worker_rank < m%worker_size?1:0);  // количество элементов на отдельном процессе
        int index = 0;  // глобальный индекс, элементы [index, index+portion*n) хранятся на одном процессе
        if (worker_rank < m%worker_size) {
            index = portion*worker_rank*n;
        } else {
            index = (portion+1)*(m%worker_size)*n + portion*(worker_rank-m%worker_size)*n;
        }  
        // инициализация
        for (int i = index; i < index + portion*n; i++) {
            pv.set_elem(i, i);
        }  
        for (int i = 0; i < n; i++)
            b[i] = i;
        mm.change_mode(READ_ONLY);
        std::vector<int>tmp_ans(portion);
        int t = 0;
        for (int i = index/n; i < index/n+portion; i++) {
            tmp_ans[t] = 0;
            for (int j = 0; j < n; j++) {
                tmp_ans[t] += pv.get_elem(i*n + j)*b[j];
            }
            t++;
        }
        mm.change_mode(READ_WRITE);  // копирование элементов в вектор, с которого можно будет получить данные на любом процессе
        for (int i = index/n; i < index/n + portion; i++) {
            ans.set_elem(i, tmp_ans[i-index/n]);
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    double t3 = MPI_Wtime();
    if (rank == 1) {
        std::cout<<t3-t1<<"\n";
    }
    mm.finalize();
    return 0;
}
