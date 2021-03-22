#ifndef __PARALLEL_VECTOR_H__
#define __PARALLEL_VECTOR_H__

#include <vector>
#include <mpi.h>
#include "common.h"
#include "memory_manager.h"

template<class T>
class parallel_vector {
    int key;  // идентификатор вектора в memory_manager
    int size_vector;  // глобальный размер вектора
public:
    parallel_vector(const int& size=DEFAULT_QUANTUM_SIZE, const int& quantum_size = DEFAULT_QUANTUM_SIZE);
    int get_elem(const int& index) const;  // получить элемент по глобальному индексу
    void set_elem(const int& index, const int& value);  // сохранить элемент по глобальному индексу
    void set_lock(int quantum_index);  // заблокировать квант
    void unset_lock(int quantum_index);  // разблокировать квант
    int get_quantum(int index);  // по глобальному индексу получить номер кванта
    int get_key() const;  // получить идентификатор вектора в memory_manager
    int get_num_quantums() const;
    int get_quantum_size() const;
    int size() const;
    void read(const std::string& path, int number_of_elements);
    void read(const std::string& path, int number_of_elements, int offset, int num_elem_proc);
    void print(const std::string& path) const;
    void change_mode(int quantum_index, int mode);
    void change_mode(int quantum_index_l, int quantum_index_r, int mode);
};

template<class T>
parallel_vector<T>::parallel_vector(const int& number_of_elems, const int& quantum_size) {
    key = memory_manager::create_object<T>(number_of_elems, quantum_size);
    size_vector = number_of_elems;
}

template<class T>
int parallel_vector<T>::get_elem(const int& index) const {
    if(index < 0 || index >= size_vector)
        throw -1;
    return memory_manager::get_data<T>(key, index);
}

template<class T>
void parallel_vector<T>::set_elem(const int& index, const int& value) {
    if(index < 0 || index >= size_vector)
        throw -1;
    memory_manager::set_data<T>(key, index, value);
}

template<class T>
void parallel_vector<T>::set_lock(int quantum_index) {
    memory_manager::set_lock(key, quantum_index);
}

template<class T>
void parallel_vector<T>::unset_lock(int quantum_index) {
    memory_manager::unset_lock(key, quantum_index);
}

template<class T>
int parallel_vector<T>::get_quantum(int index) {
    return memory_manager::get_quantum_index(key, index);
}

template<class T>
int parallel_vector<T>::get_key() const {
    return key;
}

template<class T>
int parallel_vector<T>::get_num_quantums() const {
    return (size_vector + memory_manager::get_quantum_size(key) - 1) / memory_manager::get_quantum_size(key);
}

template<class T>
int parallel_vector<T>::get_quantum_size() const {
    return memory_manager::get_quantum_size(key);
}

template<class T>
int parallel_vector<T>::size() const {
    return size_vector;
}

template<class T>
void parallel_vector<T>::read(const std::string& path, int number_of_elements) {
    memory_manager::read<T>(key, path, number_of_elements);
}

template<class T>
void parallel_vector<T>::read(const std::string& path, int number_of_elements, int offset, int num_elem_proc) {
    memory_manager::read<T>(key, path, number_of_elements, offset, num_elem_proc);
}

template<class T>
void parallel_vector<T>::print(const std::string& path) const {
    memory_manager::print(key, path);
}

template<class T>
void parallel_vector<T>::change_mode(int quantum_index, int mode) {
    memory_manager::change_mode(key, quantum_index, quantum_index+1, mode);
}

template<class T>
void parallel_vector<T>::change_mode(int quantum_index_l, int quantum_index_r, int mode) {
    memory_manager::change_mode(key, quantum_index_l, quantum_index_r, mode);
}

#endif // __PARALLEL_VECTOR_H__
