#ifndef __PARALLEL_VECTOR_H__
#define __PARALLEL_VECTOR_H__

#include <vector>
#include <mpi.h>
#include "memory_manager.h"

class parallel_vector {
    int key;  // идентификатор вектора в memory_manager
    int size_vector;  // глобальный размер вектора
    int size_proc, rank_proc;
    int portion;
public:
    parallel_vector();
    parallel_vector(const int& size);
    int get_elem(const int& index) const;  // получить элемент по глобальному индексу
    void set_elem(const int& index, const int& value);  // сохранить элемент по глобальному индексу
    void set_lock(int quantum_index);  // заблокировать квант
    void unset_lock(int quantum_index);  // разблокировать квант
    int get_quantum(int index);  // по глобальному индексу получить номер кванта
    int get_key() const;  // получить идентификатор вектора в memory_manager
};

#endif // __PARALLEL_VECTOR_H__