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
    MPI_Comm_size(MPI_COMM_WORLD, &size_proc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_proc);
    key = mm.create_object(number_of_elems);
    size_vector = number_of_elems; 
}

parallel_vector::parallel_vector(const parallel_vector& pv) {
    portion = pv.portion;
    size_vector = pv.size_vector;
    size_proc = pv.size_proc;
    rank_proc = pv.rank_proc;
    key = mm.create_object(size_vector);
    mm.copy_data(pv.key, key);
}

parallel_vector& parallel_vector::operator=(const parallel_vector& pv) {
    if(this != &pv) {
        portion = pv.portion;
        size_vector = pv.size_vector;
        size_proc = pv.size_proc;
        rank_proc = pv.rank_proc;
        mm.copy_data(pv.key, key);
    }
    return *this;
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

int parallel_vector::get_elem_proc(const int& index) const { // WARNING!
    if(index < 0 || index >= mm.get_size_of_portion(key))
         throw -1;
    return mm.get_data_by_index_on_process(key, index);
}

void parallel_vector::set_elem_proc(const int& index, const int& value) { // WARNING!
    if(index < 0 || index >= mm.get_size_of_portion(key))
         throw -1;
    mm.set_data_by_index_on_process(key, index, value);
}

int parallel_vector::get_portion() const {
    return mm.get_size_of_portion(key);
}

int parallel_vector::get_index_of_process(const int& index) const {
    return mm.get_number_of_process(key, index);
}

int parallel_vector::get_index_of_element(const int& index) const {
    return mm.get_number_of_element(key, index);
}

int parallel_vector::get_logical_index_of_element(const int& index, const int& process)  const {
    return mm.get_logical_index_of_element(key, index, process);
}

void parallel_vector::set_lock_read(int quantum_index) {
    mm.set_lock_read(key, quantum_index);
}

void parallel_vector::set_lock_write(int quantum_index) {
    mm.set_lock_write(key, quantum_index);
}

void parallel_vector::unset_lock(int quantum_index) {
    mm.unset_lock(key, quantum_index);   
}

int parallel_vector::get_quantum(int index) {
    return mm.get_quantum_index(index);
}