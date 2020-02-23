#ifndef __PARALLEL_VECTOR_H__
#define __PARALLEL_VECTOR_H__

#include <vector>
#include <mpi.h>
#include "memory_manager.h"

class parallel_vector {
    // std::vector<int>v;
    int key;
    int size_vector;
    int size_proc, rank_proc;
    int portion;
public:
    // parallel_vector();
    parallel_vector(const int& size);
    parallel_vector(const parallel_vector& v);
    parallel_vector& operator=(const parallel_vector& v);
    int get_elem(const int& index) const;
    void set_elem(const int& index, const int& value);
    // int get_portion() const;
    // int get_elem_proc(const int& index) const; // don't touch this in main!
    // void set_elem_proc(const int& index, const int& value); //don't touch this in main! 
    // int get_index_of_proccess(const int& index) const;
    // int get_index_of_element(const int& index) const;
    // int get_reverse_index_of_element(const int& index, const int& proccess) const;
};

#endif // __PARALLEL_VECTOR_H__