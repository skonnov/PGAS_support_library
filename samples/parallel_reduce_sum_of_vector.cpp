#include <iostream>
#include <mpi.h>
#include "parallel_vector.h"
#include "parallel_for.h"
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
    MPI_Init(&argc, &argv);
    int size, rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    parallel_for pf;
    int n;
    if(argc == 1)
        n = 10;
    else
        n = atoi(argv[1]);
    parallel_vector pv(n);
    for(int i = 0; i < n; i++)
        pv.set_elem(i, i);
    // for(int i = 0; i < n; i++) {
    //     int ans = pv.get_elem(i);
    //     if(rank == 0)
    //         std::cout<<ans<<" ";
    // }
    // if(rank == 0)
    //     std::cout<<"\n";
    // pf(3, 4, pv, [](int a){return a + 5;});
    // MPI_Barrier(MPI_COMM_WORLD);
    // for(int i = 0; i < n; i++) {
    //     int ans = pv.get_elem(i);
    //     if(rank == 0)
    //         std::cout<<ans<<" ";
    // }
    if(rank == 0)
        std::cout<<"\n";
    double t1 = MPI_Wtime();
    int ans = parallel_reduce(0, n, pv, 0, Func(pv), reduction);
    double t2 = MPI_Wtime();
    if(rank == 0)
        std::cout<<t2-t1;
    MPI_Finalize();
}
