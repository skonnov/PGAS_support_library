#include <iostream>
#include <vector>
#include <mpi.h>

using namespace std;

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    vector<int>v;
    int n = atoi(argv[1]);
    for(int i = 0; i < n; i++) {
        v.push_back(i);
    }
    double t1 = MPI_Wtime();
    int sum = 0;
    for(int i = 0; i < n; i++)
        sum+=v[i];
    double t2 = MPI_Wtime();
    cout<<t2-t1;
    MPI_Finalize();
    return 0;
}