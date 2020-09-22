#include <iostream>
#include <vector>
#include <mpi.h>
#include <cassert>
#include "parallel_vector.h"
#include "memory_manager.h"

parallel_vector::parallel_vector(const int& number_of_elems) {
    key = memory_manager::create_object(number_of_elems);
    size_vector = number_of_elems; 
}

int parallel_vector::get_elem(const int& index) const {
    if(index < 0 || index >= size_vector)
        throw -1;
    return memory_manager::get_data(key, index);
}

void parallel_vector::set_elem(const int& index, const int& value) {
    if(index < 0 || index >= size_vector)
        throw -1;
    memory_manager::set_data(key, index, value);
}

void parallel_vector::set_lock(int quantum_index) {
    memory_manager::set_lock(key, quantum_index);
}

void parallel_vector::unset_lock(int quantum_index) {
    memory_manager::unset_lock(key, quantum_index);
}

int parallel_vector::get_quantum(int index) {
    return memory_manager::get_quantum_index(index);
}

int parallel_vector::get_key() const {
    return key;
}

void parallel_vector::change_mode(int index, int mode) {
    memory_manager::change_mode(key, get_quantum(index), mode);
}