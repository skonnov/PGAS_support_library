#include "parallel_priority_queue.h"
#include "parallel_reduce.h"
#include <algorithm>
#include <climits>

parallel_priority_queue::parallel_priority_queue(int _num_of_quantums_proc, int _quantum_size, int _default_value) {
    worker_rank = memory_manager::get_MPI_rank()-1;
    worker_size = memory_manager::get_MPI_size()-1;

    num_of_quantums_proc = _num_of_quantums_proc;
    quantum_size = _quantum_size;
    default_value = _default_value;

    num_of_elems_proc = num_of_quantums_proc*quantum_size;
    global_index_l = worker_rank*(num_of_elems_proc);

    maxes = parallel_vector<int>(worker_size, 1);
    sizes = parallel_vector<int>(worker_size, 1);

    pqueues = parallel_vector<int>(worker_size*num_of_elems_proc, quantum_size);

    if (worker_rank >= 0) {
        maxes.set_elem(worker_rank, default_value);
        sizes.set_elem(worker_rank, 0);
        for (int i = global_index_l; i < global_index_l + num_of_elems_proc; i++)
            pqueues.set_elem(i, default_value);
    }

    MPI_Barrier(MPI_COMM_WORLD);
}

void parallel_priority_queue::insert(int elem) {  // need to be called by all processes
    // TODO: check rank w/ min size of priority_queue through parallel_reduce
    // need to update parallel_reduce
    int id_min = 0;
    int minn = INT_MAX;
    if (worker_rank >= 0) {
        for (int i = 0; i < worker_size; i++) {
            int size_i = sizes.get_elem(i);
            if (size_i < minn) {
                id_min = i;
                minn = size_i;
            }
        } // bad, need smth else
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if(worker_rank == id_min)
        insert_internal(elem);
    MPI_Barrier(MPI_COMM_WORLD);
}

void parallel_priority_queue::insert_internal(int elem) {
    int sizes_worker = sizes.get_elem(worker_rank);
    int current_index = sizes_worker;
    if (sizes_worker == num_of_elems_proc)
        throw -1;
    pqueues.set_elem(current_index + global_index_l, elem);
    int parent_index = (current_index - 1) / 2;
    while(parent_index >= 0 && current_index > 0) {
        int parent_value = pqueues.get_elem(parent_index + global_index_l);
        int cur_value = pqueues.get_elem(current_index + global_index_l);
        if (cur_value > parent_value) {
            pqueues.set_elem(parent_index + global_index_l, cur_value);
            pqueues.set_elem(current_index + global_index_l, parent_value);
        }
        current_index = parent_index;
        parent_index = (current_index - 1)/2;
    }
    maxes.set_elem(worker_rank, pqueues.get_elem(global_index_l));
    sizes.set_elem(worker_rank, sizes_worker + 1);
}

class Func {
    parallel_vector<int>* a;
public:
    Func(parallel_vector<int>& pv) {
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

void parallel_priority_queue::remove_max() {
    int maxx = default_value;
    int id_max = 0;
    bool empty = true;
    if (worker_rank >= 0) {
        for (int i = 0; i < worker_size; i++) {
            int size = sizes.get_elem(i);
            if(size) {
                int max_i = maxes.get_elem(i);
                if (max_i > maxx) {
                    id_max = i;
                    maxx = max_i;
                }
                empty = false;
            }
        } // bad, need smth else
    }
    if(worker_rank >= 0 && empty) {
        throw -1;
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if (worker_rank == id_max) {
        remove_max_internal();
    }
    MPI_Barrier(MPI_COMM_WORLD);
}

void parallel_priority_queue::heapify(int index) {
    int left = 2 * index + 1, right = 2 * index + 2;
    int size = sizes.get_elem(worker_rank);
    if (left < size) {
        int parent_value = pqueues.get_elem(index + global_index_l);
        int left_value = pqueues.get_elem(left + global_index_l);
        if(parent_value < left_value) {
            pqueues.set_elem(index + global_index_l, left_value);
            pqueues.set_elem(left + global_index_l, parent_value);
            heapify(left);
        }
    }
    if (right < size) {
        int parent_value = pqueues.get_elem(index + global_index_l);
        int right_value = pqueues.get_elem(right + global_index_l);
        if(parent_value < right_value) {
            pqueues.set_elem(index + global_index_l, right_value);
            pqueues.set_elem(right + global_index_l, parent_value);
            heapify(right);
        }
    }
}

void parallel_priority_queue::remove_max_internal() {
    int size = sizes.get_elem(worker_rank);
    if (size == 0) {
        throw -1;
    }
    pqueues.set_elem(global_index_l, pqueues.get_elem(global_index_l + size - 1));
    sizes.set_elem(worker_rank, size - 1);
    heapify(0);
    if (size - 1 == 0)
        maxes.set_elem(worker_rank, default_value);
    else
        maxes.set_elem(worker_rank, pqueues.get_elem(global_index_l));
}
