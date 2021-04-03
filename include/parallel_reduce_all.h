#ifndef __PARALLEL_REDUCE_ALL_H__
#define __PARALLEL_REDUCE_ALL_H__

#include <functional>
#include <mpi.h>
#include <iostream>
#include "common.h"
#include "parallel_vector.h"

bool hi() {
    return true;
}

// функция объединения данных и рассылки по всем процессам
// аргументы: data - значение, полученное после выполнения функции parallel_reduce_all;
// reduction – функция объединения данных с двух процессов;
// process_begin, process_end – номера участвующих в редукции процессов;
template<class Reduction, class T>
T reduce_all_operation(T data, const Reduction& reduction, int process_begin, int process_end, MPI_Datatype type) {
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    T tmpans = data;
    int tmprank = 0;
    int t = 0;
    std::vector<int>vtmprank(size+100);
    for(int i = process_begin; i <= process_end; i++) {
        vtmprank[t++] = i;
        if(rank == i)
            tmprank = t-1;
    }  // создание временной нумерации (для удобства обращения к процессам при дальнейшей работе функции reduce_all_operation)
    int n = 1;
    while(n < t)
        n *= 2;
    for(int i = 1; i < n; i = i * 2) {
        if(tmprank * 2*i < n) {
            MPI_Status status;
            T tmp;
            int sender = vtmprank[tmprank + n/(2*i)];
            if(tmprank + n/(2*i) >= t)
                continue;
            MPI_Recv(&tmp, 1, type, sender, REDUCE_TAG, MPI_COMM_WORLD, &status);
            tmpans = reduction(tmp, tmpans);
        }
        else
        {
            int destination = vtmprank[tmprank - n/(2*i)];
            MPI_Send(&tmpans, 1, type, destination, REDUCE_TAG, MPI_COMM_WORLD);
            break;
        }
    }
    T ans = tmpans;
    for(int i = 1; i < n; i = i * 2) {
        if(tmprank < i) {
            if (tmprank + i >= t)
                break;
            int destination = vtmprank[tmprank+i];
            MPI_Send(&ans, 1, type, destination, REDUCE_ALL_TAG, MPI_COMM_WORLD);
        }
        else if (tmprank < 2 * i)
        {
            int source = vtmprank[tmprank - i];
            MPI_Status status;
            MPI_Recv(&ans, 1, type, source, REDUCE_ALL_TAG, MPI_COMM_WORLD, &status);
        }
    }

    return ans;
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
    return reduce_all_operation(ans, reduction, process_begin, process_end, pv.get_MPI_datatype());
}

#endif // __PARALLEL_REDUCE_ALL_H__
