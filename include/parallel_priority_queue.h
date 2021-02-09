#ifndef __PARALLEL_PRIORITY_QUEUE__
#define __PARALLEL_PRIORITY_QUEUE__

#include <climits>
#include "parallel_vector.h"
#include "memory_manager.h"
#include "common.h"

class parallel_priority_queue {
    int worker_rank, worker_size;
    parallel_vector maxes, sizes, pqueues;  // максимальные элементы на каждом процессе, размер приоритетной очереди на каждом процессе, вектор для храненения приоритетной очереди
    int num_of_quantums_proc, quantum_size, num_of_elems_proc;  // число квантов на одном процессе, размер кванта, число элементов на одном процессе
    int global_index_l;  // смещение в pqueues от начала в глобальной памяти
    int default_value;
public:
    parallel_priority_queue(int _num_of_quantums_proc, int _quantum_size=DEFAULT_QUANTUM_SIZE, int _default_value = INT_MIN);
    void insert(int elem);
    int get_max(int rank);
    void remove_max();
private:
    void remove_max_internal();
    void insert_internal(int elem);
    void heapify(int index);  // переупорядочивание элементов в приоритетной очереди на одном процессе
};

#endif
