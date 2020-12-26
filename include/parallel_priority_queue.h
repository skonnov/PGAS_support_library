#ifndef __PARALLEL_PRIORITY_QUEUE__
#define __PARALLEL_PRIORITY_QUEUE__

#include <climits>
#include "parallel_vector.h"
#include "memory_manager.h"
#include "common.h"

class parallel_priority_queue {
    int worker_rank, worker_size;
    parallel_vector maxes, sizes, pqueues;
    int num_of_quantums_proc, quantum_size, num_of_elems_proc;
    int global_index_l;
    int default_value;
public:
    parallel_priority_queue(int _num_of_quantums_proc, int _quantum_size=DEFAULT_QUANTUM_SIZE, int _default_value = INT_MIN);
    void insert(int elem);
    int get_max(int rank);
    void remove_max();
private:
    void remove_max_internal();
    void insert_internal(int elem);
    void heapify(int index);
};

#endif
