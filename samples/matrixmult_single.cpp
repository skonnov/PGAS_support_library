#include <vector>
#include <iostream>
#include "mpi.h"

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    if(argc <= 1) {
        throw -1;
    }
    int n = atoi(argv[1]);
    std::vector<int> v1(n*n), v2(n*n), ans(n*n, 0); // v2 - transposed matrix
    double t1 = MPI_Wtime();
    for(int i = 0; i < n*n; i++) {
        v1[i] = v2[i] = i;
    }
    for(int i = 0; i < n; i++) {
        for(int j = 0; j < n; j++) {
            for(int k = 0; k < n; k++) {
                ans[i*n+j] += v1[i*n+k]*v2[k*n+j];
            }
        }
    }
    double t2 = MPI_Wtime();
    std::cout<<t2-t1<<"\n";
    // for(int i = 0; i < n; i++) {
    //     for(int j = 0; j < n; j++) {
    //         std::cout<<ans[i*n+j]<<" ";
    //     }
    //     std::cout<<"\n";
    // }
    MPI_Finalize();
    return 0;
}