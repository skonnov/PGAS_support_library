#ifndef __PARALLEL_PRIORITY_QUEUE__
#define __PARALLEL_PRIORITY_QUEUE__

#include <climits>
#include <algorithm>
#include <cstddef>
#include "common.h"
#include "parallel_vector.h"
#include "parallel_reduce.h"
#include "parallel_reduce_all.h"
#include "memory_manager.h"

struct pair_reduce {
    int first, second;

    pair_reduce() {}
    pair_reduce(const int& a, const int& b) {
        first = a;
        second = b;
    }
};

template <class T>
class parallel_priority_queue {
    int worker_rank, worker_size;
    parallel_vector<T> pqueues;
    parallel_vector<T> maxes;
    parallel_vector<int> sizes;  // максимальные элементы на каждом процессе, размер приоритетной очереди на каждом процессе, вектор для храненения приоритетной очереди
    int num_of_quantums_proc, quantum_size, num_of_elems_proc;  // число квантов на одном процессе, размер кванта, число элементов на одном процессе
    int global_index_l;  // смещение в pqueues от начала в глобальной памяти
    T default_value;
    MPI_Datatype pair_type;
public:
    parallel_priority_queue(T _default_value, int _num_of_quantums_proc, int _quantum_size=DEFAULT_QUANTUM_SIZE);
    void insert(T elem);
    void insert(T elem, int from_worker_rank);
    void insert_local(T elem);
    int get_size();
    T get_max();
    T get_max(int rank);
    // T get_and_remove_max();
    void remove_max();
private:
    void remove_max_local();
    void heapify(int index);  // переупорядочивание элементов в приоритетной очереди на одном процессе
};

template<class T>
parallel_priority_queue<T>::parallel_priority_queue(T _default_value, int _num_of_quantums_proc, int _quantum_size) {
    worker_rank = memory_manager::get_MPI_rank()-1;
    worker_size = memory_manager::get_MPI_size()-1;

    num_of_quantums_proc = _num_of_quantums_proc;
    quantum_size = _quantum_size;
    default_value = _default_value;

    num_of_elems_proc = num_of_quantums_proc*quantum_size;
    global_index_l = worker_rank*(num_of_elems_proc);

    maxes = parallel_vector<T>(worker_size, 1);
    sizes = parallel_vector<int>(worker_size, 1);

    pqueues = parallel_vector<T>(worker_size*num_of_elems_proc, quantum_size);

    if (worker_rank >= 0) {
        maxes.set_elem(worker_rank, default_value);
        sizes.set_elem(worker_rank, 0);
        for (int i = global_index_l; i < global_index_l + num_of_elems_proc; i++)
            pqueues.set_elem(i, default_value);
    }

    int count = 2;
    int blocklens[] = {1, 1};
    MPI_Aint indices[] = {
        (MPI_Aint)offsetof(pair_reduce, first),
        (MPI_Aint)offsetof(pair_reduce, second)
    };
    MPI_Datatype types[] = { MPI_INT, MPI_INT };
    pair_type = create_mpi_type<pair_reduce>(2, blocklens, indices, types);
    MPI_Barrier(MPI_COMM_WORLD);
}

template<class T, class T2>
class Func1 {
    parallel_vector<T>* a;
public:
    Func1(parallel_vector<T>& pv) {
        a = &pv;
    }
    T2 operator()(int l, int r, T2 identity) const {
        return { a->get_elem(l), memory_manager::get_MPI_rank()-1 };
    }
};

template<class T>
void parallel_priority_queue<T>::insert(T elem) {
    auto reduction = [](pair_reduce a, pair_reduce b) { return (a.first < b.first) ? a : b; };
    pair_reduce size{-2, -2};
    if (worker_rank >= 0)
        size = parallel_reduce_all(worker_rank, worker_rank+1, sizes, pair_reduce(INT_MAX, INT_MAX), 1, worker_size, Func1<int, pair_reduce>(sizes), reduction, pair_type);
    if(worker_rank == size.second)
        insert_local(elem);
}

template<class T>
void parallel_priority_queue<T>::insert(T elem, int from_worker_rank) {
    auto reduction = [](pair_reduce a, pair_reduce b) { return (a.first < b.first) ? a : b; };
    pair_reduce size{-2, -2};
    if (worker_rank >= 0)
        size = parallel_reduce_all(worker_rank, worker_rank+1, sizes, pair_reduce(INT_MAX, INT_MAX), 1, worker_size, Func1<int, pair_reduce>(sizes), reduction, pair_type);
    auto func = [elem](int begin, int end, T identity) { return elem; };
    auto reduction2 = [](int a, int b) { return b; };
    if (worker_rank >= 0)
        elem = parallel_reduce(0, 0, sizes, 0, from_worker_rank+1, from_worker_rank+1, func, reduction2, size.second+1);
    if (worker_rank == size.second)
        insert_local(elem);
}

template<class T>
int parallel_priority_queue<T>::get_size() {
    auto func = [this](int begin, int end, int identity) { return sizes.get_elem(begin); };
    auto reduction = [](int a, int b){ return a+b; };
    int size = parallel_reduce_all(worker_rank, worker_rank+1, sizes, 0, 1, worker_size, func, reduction);
    return size;
}

template<class T>
void parallel_priority_queue<T>::insert_local(T elem) {
    int sizes_worker = sizes.get_elem(worker_rank);
    int current_index = sizes_worker;
    CHECK(sizes_worker < num_of_elems_proc, ERR_UNKNOWN);
    pqueues.set_elem(current_index + global_index_l, elem);
    int parent_index = (current_index - 1) / 2;
    while(parent_index >= 0 && current_index > 0) {
        T parent_value = pqueues.get_elem(parent_index + global_index_l);
        T cur_value = pqueues.get_elem(current_index + global_index_l);
        if (parent_value < cur_value) {
            pqueues.set_elem(parent_index + global_index_l, cur_value);
            pqueues.set_elem(current_index + global_index_l, parent_value);
        }
        current_index = parent_index;
        parent_index = (current_index - 1)/2;
    }
    maxes.set_elem(worker_rank, pqueues.get_elem(global_index_l));
    sizes.set_elem(worker_rank, sizes_worker + 1);
}

template<class T>
class Func {
    parallel_vector<T>* a;
public:
    Func(parallel_vector<T>& pv) {
        a = &pv;
    }
    T operator()(int l, int r, T identity) const {
        return a->get_elem(l);
    }
};

template<class T>
T parallel_priority_queue<T>::get_max(int rank) {
    auto reduction = [](T a, T b){return std::max(a, b);};
    return parallel_reduce(worker_rank, worker_rank+1, maxes, default_value, 1, worker_size /*global_size*/, Func<T>(maxes), reduction, rank /*global_rank*/);
}

template<class T>
T parallel_priority_queue<T>::get_max() {
    auto reduction = [](T a, T b){return std::max(a, b);};
    return parallel_reduce_all(worker_rank, worker_rank+1, maxes, default_value, 1, worker_size /*global_size*/, Func<T>(maxes), reduction);
}

// template<class T>
// T parallel_priority_queue<T>::get_and_remove_max() {
//     auto reduction = [](pair_reduce a, pair_reduce b) { return (a.first >= b.first) ? a : b; };
//     pair_reduce size{-2, -2};
//     if (worker_rank >= 0)
//         size = parallel_reduce_all(worker_rank, worker_rank+1, maxes, pair_reduce(default_value, 0), 1, worker_size, Func1<int, pair_reduce(sizes), reduction, pair_type, rank);
//     if(worker_rank == size.second) {
//         remove_max_local();
//     }
//     return size.first;
// }

template<class T>
void parallel_priority_queue<T>::remove_max() {
    auto reduction = [](pair_reduce a, pair_reduce b) { return (a.first >= b.first) ? a : b; };
    pair_reduce size{-2, -2};
    if (worker_rank >= 0)
        size = parallel_reduce_all(worker_rank, worker_rank+1, maxes, pair_reduce(INT_MAX, INT_MAX), 1, worker_size, Func1<int, pair_reduce>(sizes), reduction, pair_type);
    if (worker_rank == size.second) {
        remove_max_local();
    }
}

template<class T>
void parallel_priority_queue<T>::heapify(int index) {
    int left = 2 * index + 1, right = 2 * index + 2;
    int size = sizes.get_elem(worker_rank);
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
void parallel_priority_queue<T>::remove_max_local() {
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


#endif
