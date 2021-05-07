#ifndef __PARALLEL_REDUCE_ALL_H__
#define __PARALLEL_REDUCE_ALL_H__

#include <functional>
#include <mpi.h>
#include <iostream>
#include "common.h"
#include "parallel_vector.h"

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
        if(tmprank * 2 * i < n) {
            if(tmprank + n / (2 * i) >= t)
                continue;
            MPI_Status status;
            T tmp;
            int sender = vtmprank[tmprank + n/(2*i)];
            MPI_Recv(&tmp, sizeof(T), MPI_BYTE, sender, REDUCE_ALL_TAG1, MPI_COMM_WORLD, &status);
            tmpans = reduction(tmp, tmpans);
        }
        else
        {
            int destination = vtmprank[tmprank - n/(2*i)];
            MPI_Send(&tmpans, sizeof(T), MPI_BYTE, destination, REDUCE_ALL_TAG1, MPI_COMM_WORLD);
            break;
        }
    }
    T ans = tmpans;
    for(int i = 1; i < n; i = i * 2) {
        if(tmprank < i) {
            if (tmprank + i >= t)
                break;
            int destination = vtmprank[tmprank+i];
            MPI_Send(&ans, sizeof(T), MPI_BYTE, destination, REDUCE_ALL_TAG2, MPI_COMM_WORLD);
        }
        else if (tmprank < 2 * i)
        {
            int sender = vtmprank[tmprank - i];
            MPI_Status status;
            MPI_Recv(&ans, sizeof(T), MPI_BYTE, sender, REDUCE_ALL_TAG2, MPI_COMM_WORLD, &status);
        }
    }
    return ans;
}


// аргументы: аргументы: [l, r] – диапазон глобальных индексов на отдельном процессе;
// pv – вектор, над которым осуществляется функция редукции;
// process_begin, process_end – номера участвующих в редукции процессов;
// func – функтор, используемый для сбора данных на одном процессе;
// reduction – функтор, используемый для объединения данных с разных процессов;
// process – номер процесса, на котором редуцируются данные
template<class Func, class Reduction, class T, class T2>
T parallel_reduce_all(int l, int r, const parallel_vector<T2>& pv, T identity, int process_begin, int process_end, const Func& func, const Reduction& reduction) {
    T ans = identity;
    ans = func(l, r, identity);
    return reduce_all_operation(ans, reduction, process_begin, process_end, pv.get_MPI_datatype());
}

template<class Func, class Reduction, class T, class T2>
T parallel_reduce_all(int l, int r, const parallel_vector<T2>& pv, T identity, int process_begin, int process_end, const Func& func, const Reduction& reduction, MPI_Datatype type) {
    T ans = identity;
    ans = func(l, r, identity);
    return reduce_all_operation(ans, reduction, process_begin, process_end, type);
}


#endif // __PARALLEL_REDUCE_ALL_H__
