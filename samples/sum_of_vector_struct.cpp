#include <iostream>
#include <cassert>
#include <mpi.h>
#include <fstream>
#include "parallel_vector.h"
#include "memory_manager.h"

struct str {
    int a;
    double b;
    char c;
};

int main(int argc, char** argv) {
    std::string error_helper_string = "mpiexec -n <numproc> "+std::string(argv[0])+" <length>";
    if (argc <= 1) {
        std::cout << "Error: you need to pass length of vector!" << std::endl;
        std::cout << "Usage:\n" << error_helper_string << std::endl;
        return 1;
    }
    memory_manager::memory_manager_init(argc, argv, error_helper_string);

    int count = 3;
    int blocklens[] = {1, 1, 1};
    MPI_Aint indices[] = {
        (MPI_Aint)offsetof(str, a),
        (MPI_Aint)offsetof(str, b),
        (MPI_Aint)offsetof(str, c),
    };
    MPI_Datatype types[] = { MPI_INT, MPI_DOUBLE, MPI_CHAR };

    int n = atoi(argv[1]);
    parallel_vector<str> v(count, blocklens, indices, types, n, 1);
    auto rank = memory_manager::get_MPI_rank();
    if (rank) {
        for (int i = 0; i < n; i++) {
            v.set_elem(i, {i, i+0.5, char(i)});
        }
        v.change_mode(0, v.get_num_quantums(), READ_ONLY);
        str sum = { 0, 0, 0 };
        for (int i = 0; i < n; i++) {
            str tmp = v.get_elem(i);
            sum.a += tmp.a;
            sum.b += tmp.b;
            sum.c += tmp.c;
        }
        std::cout<<sum.a<<" "<<sum.b<<" "<<int(sum.c)<<" | "<<rank<<std::endl;
    }
    memory_manager::finalize();
}