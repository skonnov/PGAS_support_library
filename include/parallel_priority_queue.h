#ifndef __PARALLEL_PRIORITY_QUEUE__
#define __PARALLEL_PRIORITY_QUEUE__

#include "parallel_vector.h"
#include "memory_manager.h"
#include "common.h"

class parallel_priority_queue {
    int worker_rank, worker_size;
    parallel_vector maxes, sizes, pqueues;
    int num_of_quantums_proc, quantum_size;
    parallel_priority_queue(int _num_of_quantums_proc, int _quantum_size=DEFAULT_QUANTUM_SIZE);
    void insert(int elem);
    int get_max();
};

#endif