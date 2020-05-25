#include <iostream>
#include <vector>
#include <mpi.h>
#include <cassert>
#include "parallel_vector.h"
#include "memory_manager.h"


extern memory_manager mm;
// parallel_vector::parallel_vector() {
//     MPI_Comm_size(MPI_COMM_WORLD, &sizeproc);
//     MPI_Comm_rank(MPI_COMM_WORLD, &rankproc);
//     portion = 1; 
//     allsize = sizeproc;
//     v.resize(portion);
// }


parallel_vector::parallel_vector(const int& number_of_elems) {
    key = mm.create_object(number_of_elems);
    size_vector = number_of_elems; 
}

int parallel_vector::get_elem(const int& index) const {
    if(index < 0 || index >= size_vector)
        throw -1;
    return mm.get_data(key, index);
}

void parallel_vector::set_elem(const int& index, const int& value) {
    if(index < 0 || index >= size_vector)
        throw -1;
    mm.set_data(key, index, value);
}

void parallel_vector::set_lock(int quantum_index) {
    mm.set_lock(key, quantum_index);
}

void parallel_vector::unset_lock(int quantum_index) {
    mm.unset_lock(key, quantum_index);   
}

int parallel_vector::get_quantum(int index) {
    return mm.get_quantum_index(index);
}

int parallel_vector::get_key() const {
    return key;
}