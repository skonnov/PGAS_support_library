#include <iostream>
#include <cassert>
#include <string>
#include <mpi.h>
#include "memory_manager.h"
#include "parallel_vector.h"
#include "parallel_reduce.h"

class Func {
    parallel_vector* a;
public:
    Func(parallel_vector& pv) {
        a = &pv;
    }
    int operator()(int l, int r, int identity) const {
        int ans = identity;
        for(int i = l; i < r; i++)
            ans+=a->get_elem(i);
        return ans;
    }
};

int reduction(int a, int b)
{
    return a + b;
}

int main(int argc, char ** argv) {
    std::string error_helper_string = "mpiexec -n <numproc> "+std::string(argv[0])+" <length>";
    if (argc <= 1) {
        std::cout << "Error: you need to pass length of vector!" << std::endl;
        std::cout << "Usage:\n" << error_helper_string << std::endl;
        return 1;
    }
    memory_manager::memory_manager_init(argc, argv, error_helper_string);
    double t1 = MPI_Wtime();
    int rank = memory_manager::get_MPI_rank();
    int size = memory_manager::get_MPI_size();
    assert(argc > 1);
    int n = atoi(argv[1]);
    parallel_vector pv(n);
    if (rank != 0) {
        int worker_rank = rank-1;
        int worker_size = size-1;
        // разделение элементов вектора по процессам
        int portion = n/worker_size + (worker_rank < n%worker_size?1:0);
        int index = 0;
        if (worker_rank < n%worker_size) {
            index = portion*worker_rank;
        } else {
            index = (portion+1)*(n%worker_size) + portion*(worker_rank-n%worker_size);
        }
        for (int i = index; i < index+portion; i++)  // инициализация элементов вектора
            pv.set_elem(i, i);
        pv.change_mode(0, pv.get_num_quantums(), READ_ONLY);// так как далее вектор изменяться не будет, режим изменяется на READ_ONLY
        int ans = parallel_reduce(index, index+portion, pv, 0, 1, size-1, Func(pv), reduction);
        double t2 = MPI_Wtime();
        if(rank == 1)
            std::cout<<t2-t1<<std::flush;
    }
    memory_manager::finalize();
    return 0;
}
