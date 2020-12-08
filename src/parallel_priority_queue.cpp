#include "parallel_priority_queue.h"
#include "parallel_reduce.h"
#include <algorithm>
#include <climits>

parallel_priority_queue::parallel_priority_queue(int _num_of_quantums_proc, int _quantum_size) {
    worker_rank = memory_manager::get_MPI_rank()-1;
    worker_size = memory_manager::get_MPI_size()-1;
    num_of_quantums_proc = _num_of_quantums_proc;
    quantum_size = _quantum_size;
    num_of_elems_proc = num_of_quantums_proc*quantum_size;
    global_index_l = worker_rank*num_of_elems_proc*2;
    maxes = parallel_vector(worker_size, 1);
    cur_worker = 0;
    size_pqueue = 0;
    pqueues = parallel_vector(2*worker_size*num_of_elems_proc, quantum_size);
    if (worker_rank >= 0) {
        maxes.set_elem(worker_rank, 0);  // 0???
        for (int i = global_index_l; i < global_index_l + num_of_elems_proc * 2; i++)
            pqueues.set_elem(i, 0);  // 0???
    }
    MPI_Barrier(MPI_COMM_WORLD);
}

void parallel_priority_queue::insert(int elem) {  // need to be called by all processes
    if(worker_rank == (cur_worker++)%worker_size)
        insert_internal(elem);
}

void parallel_priority_queue::insert_internal(int elem) {
    int local_index = size_pqueue;
    local_index += num_of_elems_proc-1;
    for (int i = local_index; i > 0; i = (i-1) >> 1) {
        pqueues.set_elem(global_index_l + i, elem);
        int elem2 = pqueues.get_elem(global_index_l + ((i-1)>>1));
        if(elem2 < elem) {
            if (((i-1)>>1) == 0) {
                pqueues.set_elem(0, elem);
                maxes.set_elem(worker_rank, elem);
            }
        } else {
            break;
        }
    }
}

class Func {
    parallel_vector* a;
public:
    Func(parallel_vector& pv) {
        a = &pv;
    }
    int operator()(int l, int r, int identity) const {
        return a->get_elem(l);
    }
};

int parallel_priority_queue::get_max(int rank) {
    auto reduction = [](int a, int b){return std::max(a, b);};
    return parallel_reduce(worker_rank, worker_rank+1, maxes, INT_MIN, 1, worker_size /*global_size*/, Func(maxes), reduction, rank /*global_rank*/);
}

int parallel_priority_queue::size() const {
    return num_of_elems_proc*worker_size;
}
