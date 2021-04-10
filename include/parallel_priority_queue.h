#ifndef __PARALLEL_PRIORITY_QUEUE__
#define __PARALLEL_PRIORITY_QUEUE__

#include <climits>
#include "parallel_vector.h"
#include "parallel_reduce.h"
#include "memory_manager.h"
#include "detail.h"
#include "common.h"
#include <algorithm>
#include <climits>

template<class T>
struct info {
    T maxx;
    int size;
};

template <class T>
class parallel_priority_queue {
    int worker_rank, worker_size;
    parallel_vector<T> pqueues;
    parallel_vector<info<T>> ms;  // максимальные элементы на каждом процессе и размер приоритетной очереди на каждом процессе
    int num_of_quantums_proc, quantum_size, num_of_elems_proc;  // число квантов на одном процессе, размер кванта, число элементов на одном процессе
    int global_index_l;  // смещение в pqueues от начала в глобальной памяти
    T default_value;
public:
    parallel_priority_queue(T _default_value, int _num_of_quantums_proc, int _quantum_size=DEFAULT_QUANTUM_SIZE);
    // parallel_priority_queue(int count, const int* blocklens, const MPI_Aint* indices, const MPI_Datatype* types,
    //                         T _default_value, int _num_of_quantums_proc, int _quantum_size=DEFAULT_QUANTUM_SIZE);
    void insert(T elem);
    T get_max(int rank);
    void remove_max();
private:
    void remove_max_internal();
    void insert_internal(T elem);
    void heapify(int index);  // переупорядочивание элементов в приоритетной очереди на одном процессе
};

// template<class T>
// parallel_priority_queue<T>::parallel_priority_queue(int count, const int* blocklens, const MPI_Aint* indices, const MPI_Datatype* types,
//                             T _default_value, int _num_of_quantums_proc, int _quantum_size) {
//                             }

template<class T>
parallel_priority_queue<T>::parallel_priority_queue(T _default_value, int _num_of_quantums_proc, int _quantum_size) {
    worker_rank = memory_manager::get_MPI_rank()-1;
    worker_size = memory_manager::get_MPI_size()-1;

    num_of_quantums_proc = _num_of_quantums_proc;
    quantum_size = _quantum_size;
    default_value = _default_value;

    num_of_elems_proc = num_of_quantums_proc*quantum_size;
    global_index_l = worker_rank*(num_of_elems_proc);

    int count = 2;
    int blocklens[] = {1, 1};
    MPI_Aint indices[] = {
        (MPI_Aint)offsetof(info<T>, maxx),
        (MPI_Aint)offsetof(info<T>, size)
    };
    MPI_Datatype types[] = { get_mpi_type<T>(), MPI_INT };
    
    ms = parallel_vector<info<T>>(count, blocklens, indices, types, worker_size, 1);

    pqueues = parallel_vector<T>(worker_size*num_of_elems_proc, quantum_size);

    if (worker_rank >= 0) {
        ms.set_elem(worker_rank, {default_value, 0});
        for (int i = global_index_l; i < global_index_l + num_of_elems_proc; i++)
            pqueues.set_elem(i, default_value);
    }

    MPI_Barrier(MPI_COMM_WORLD);
}

template<class T>
void parallel_priority_queue<T>::insert(T elem) {  // need to be called by all processes
    // TODO: check rank w/ min size of priority_queue through parallel_reduce
    // need to update parallel_reduce
    int id_min = 0;
    T minn;
    if (worker_rank >= 0) {
        minn = ms.get_elem(0).size;
        for (int i = 1; i < worker_size; i++) {
            int size_i = ms.get_elem(i).size;
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

template<class T>
void parallel_priority_queue<T>::insert_internal(T elem) {
    int sizes_worker = ms.get_elem(worker_rank).size;
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
    ms.set_elem(worker_rank, { pqueues.get_elem(global_index_l), sizes_worker + 1 });
}

template<class TInfo>
class Func {
    parallel_vector<TInfo>* a;
public:
    Func(parallel_vector<TInfo>& pv) {
        a = &pv;
    }
    TInfo operator()(int l, int r, TInfo identity) const {
        return a->get_elem(l);
    }
};

template<class T>
T parallel_priority_queue<T>::get_max(int rank) {
    auto reduction = [](info<T> a, info<T> b){return (a.maxx > b.maxx) ? a : b;};
    auto ans = parallel_reduce(worker_rank, worker_rank+1, ms, { default_value, 0 }, 1, worker_size /*global_size*/, Func<info<T>>(ms), reduction, rank /*global_rank*/);
    return ans.maxx;
}

template<class T>
void parallel_priority_queue<T>::remove_max() {
    T maxx = default_value;
    int id_max = 0;
    bool empty = true;
    if (worker_rank >= 0) {
        for (int i = 0; i < worker_size; i++) {
            auto ms_elem = ms.get_elem(i);
            if(ms_elem.size) {
                if (ms_elem.maxx > maxx) {
                    id_max = i;
                    maxx = ms_elem.maxx;
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

template<class T>
void parallel_priority_queue<T>::heapify(int index) {
    int left = 2 * index + 1, right = 2 * index + 2;
    int size = ms.get_elem(worker_rank).size;
    if (left < size) {
        T parent_value = pqueues.get_elem(index + global_index_l);
        T left_value = pqueues.get_elem(left + global_index_l);
        if(parent_value < left_value) {
            pqueues.set_elem(index + global_index_l, left_value);
            pqueues.set_elem(left + global_index_l, parent_value);
            heapify(left);
        }
    }
    if (right < size) {
        T parent_value = pqueues.get_elem(index + global_index_l);
        T right_value = pqueues.get_elem(right + global_index_l);
        if(parent_value < right_value) {
            pqueues.set_elem(index + global_index_l, right_value);
            pqueues.set_elem(right + global_index_l, parent_value);
            heapify(right);
        }
    }
}

template<class T>
void parallel_priority_queue<T>::remove_max_internal() {
    auto ms_elem = ms.get_elem(worker_rank);
    if (ms_elem.size == 0) {
        throw -1;
    }
    pqueues.set_elem(global_index_l, pqueues.get_elem(global_index_l + ms_elem.size - 1));
    ms.set_elem(worker_rank, {ms_elem.maxx, ms_elem.size - 1});
    heapify(0);
    if (ms_elem.size - 1 == 0)
        ms.set_elem(worker_rank, {default_value, ms_elem.size - 1});
    else
        ms.set_elem(worker_rank, {pqueues.get_elem(global_index_l), ms_elem.size - 1});
}


#endif
