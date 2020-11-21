#include "parallel_vector.h"

parallel_vector::parallel_vector(const int& number_of_elems, const int& quantum_size) {
    key = memory_manager::create_object(number_of_elems, quantum_size);
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
    return memory_manager::get_quantum_index(key, index);
}

int parallel_vector::get_key() const {
    return key;
}

int parallel_vector::get_num_quantums() const {
    return (size_vector + memory_manager::get_quantum_size(key) - 1) / memory_manager::get_quantum_size(key);
}

int parallel_vector::get_quantum_size() const {
    return memory_manager::get_quantum_size(key);
}

void parallel_vector::read(const std::string& path, int number_of_elements) {
    memory_manager::read(key, path, number_of_elements);
}

void parallel_vector::read(const std::string& path, int number_of_elements, int offset, int num_elem_proc) {
    memory_manager::read(key, path, number_of_elements, offset, num_elem_proc);
}

void parallel_vector::print(const std::string& path) const {
    memory_manager::print(key, path);
}

void parallel_vector::change_mode(int quantum_index, int mode) {
    memory_manager::change_mode(key, quantum_index, quantum_index+1, mode);
}

void parallel_vector::change_mode(int quantum_index_l, int quantum_index_r, int mode) {
    memory_manager::change_mode(key, quantum_index_l, quantum_index_r, mode);
}