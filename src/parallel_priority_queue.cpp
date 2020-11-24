#include "parallel_priority_queue.h"
#include "parallel_reduce.h"
#include <algorithm>
#include <climits>

parallel_priority_queue::parallel_priority_queue(int _num_of_quantums_proc, int _quantum_size) {
    worker_rank = memory_manager::get_MPI_rank()-1;
    worker_size = memory_manager::get_MPI_size()-1;
    num_of_quantums_proc = _num_of_quantums_proc;
    quantum_size = _quantum_size;
    maxes = parallel_vector(worker_size, 1);
    sizes = parallel_vector(worker_size, 1);
    pqueues = parallel_vector(num_of_quantums_proc*quantum_size*worker_size, quantum_size);
}

void parallel_priority_queue::insert(int elem) {

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

int parallel_priority_queue::get_max() {
    auto reduction = [](int a, int b){return std::max(a, b);};
    return parallel_reduce(worker_rank, worker_rank+1, maxes, INT_MIN, 1, worker_size /*global_size*/, Func(maxes), reduction, worker_rank + 1 /*global_rank*/);
}