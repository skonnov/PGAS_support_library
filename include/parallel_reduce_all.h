#ifndef __PARALLEL_REDUCE_H__
#define __PARALLEL_REDUCE_H__

#include <functional>
#include <mpi.h>
#include <iostream>
#include "common.h"
#include "parallel_vector.h"
  // std::function<int(int, int)>reduction - ?
  // std::function<int(int, int, const parallel_vector&, int)> func - ?


// функция объединения данных и рассылки по всем процессам
// аргументы: ans - значение, полученное после выполнения функции parallel_reduce; 
// reduction – функция объединения данных с двух процессов;
// process_begin, process_end – номера участвующих в редукции процессов;
template<class Reduction, class T>
T reduce_all_operation(T ans, const Reduction& reduction, int process_begin, int process_end, MPI_Datatype type) {
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    T tmpans = ans;
    return tmpans;
}


// аргументы: аргументы: [l, r) – диапазон глобальных индексов на отдельном процессе;
// pv – вектор, над которым осуществляется функция редукции;
// process_begin, process_end – номера участвующих в редукции процессов;
// func – функтор, используемый для сбора данных на одном процессе;
// reduction – функтор, используемый для объединения данных с разных процессов;
// process – номер процесса, на котором редуцируются данные
template<class Func, class Reduction, class T>
T parallel_reduce_all(int l, int r, const parallel_vector<T>& pv, T identity, int process_begin, int process_end, const Func& func, const Reduction& reduction) {
    T ans = identity;
    ans = func(l, r, identity);
    return reduce_operation(ans, reduction, process_begin, process_end, pv.get_MPI_datatype());
}

#endif // __PARALLEL_REDUCE_H__
