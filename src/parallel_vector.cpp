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

// parallel_vector::parallel_vector(const parallel_vector& pv) {
//     portion = pv.portion;
//     rankproc = pv.rankproc;
//     sizeproc = pv.sizeproc;
//     allsize = pv.allsize;
//     v.resize(pv.v.size());
//     for(int i = 0; i < portion; i++) {
//         v[i] = pv.v[i];
//     }
// }

// parallel_vector& parallel_vector::operator=(const parallel_vector& pv) {
//     if(this != &pv) {
//         portion = pv.portion;
//         rankproc = pv.rankproc;
//         sizeproc = pv.sizeproc;
//         allsize = pv.allsize;
//         v.resize(pv.v.size());
//         for(int i = 0; i < portion; i++) {
//             v[i] = pv.v[i];
//         }
//     }
//     return *this;
// }

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

// int parallel_vector::get_elem_proc(const int& index) const { // WARNING!
//     if(index < 0 || index >= portion)
//          throw -1;
//     return v[index];
// }

// void parallel_vector::set_elem_proc(const int& index, const int& value) { // WARNING!
//     if(index < 0 || index >= portion)
//          throw -1;
//     v[index] = value;
// }

// int parallel_vector::get_portion() const {
//     return portion;
// }

// int parallel_vector::get_index_of_proccess(const int& index) const {
//     int number_proc;
//     if(index < (allsize%sizeproc)*(allsize/sizeproc+1)) {
//         number_proc = index/(allsize/sizeproc+1);
//     } else {
//         int tmp = index - (allsize%sizeproc)*(allsize/sizeproc+1);
//         number_proc = tmp/(allsize/sizeproc) + (allsize%sizeproc);
//     }
//     return number_proc;
// }

// int parallel_vector::get_index_of_element(const int& index) const {
//     int number_elem;
//     if(index < (allsize%sizeproc)*(allsize/sizeproc+1)) {
//         number_elem = index%(allsize/sizeproc+1);
//     } else {
//         int tmp = index - (allsize%sizeproc)*(allsize/sizeproc+1);
//         number_elem = tmp%(allsize/sizeproc);
//     }
//     return number_elem;
// }

// int parallel_vector::get_reverse_index_of_element(const int& index, const int& proccess)  const {
//     int number_elem;
//     if(proccess < allsize%sizeproc) {
//         number_elem = proccess*portion + index;
//     }
//     else {
//         number_elem = (allsize%sizeproc)*(portion+1) + (proccess-allsize%sizeproc)*portion + index;
//     }
//     return number_elem;
// }