#ifndef __PARALLEL_REDUCE_H__
#define __PARALLEL_REDUCE_H__

#include <functional>
#include <mpi.h>
#include <iostream>
#include "parallel_vector.h"
  // std::function<int(int, int)>reduction - ?
  // std::function<int(int, int, const parallel_vector&, int)> func - ?
template<class Reduction>
int reduce_operation(int ans, const Reduction& reduction, int proccess_begin, int proccess_end, int proccess = 0) {
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int tmpans = ans;
    int tmprank = 0;
    int t = 1;
    std::vector<int>vtmprank(size+100);
    vtmprank[0] = proccess;
    for(int i = proccess_begin; i <= proccess_end; i++) {
        if(i == proccess)
            continue;
        vtmprank[t++] = i;
        if(rank == i)
            tmprank = t-1;
    }
    int n = 1;
    while(n < t)
        n *= 2;
    for(int i = 1; i < n; i = i * 2) {
        if(tmprank * 2*i < n) {
            MPI_Status status;
            int tmp;
            int sender = vtmprank[tmprank + n/(2*i)];
            if(tmprank + n/(2*i) >= t)
                continue;
            MPI_Recv(&tmp, 1, MPI_INT, sender, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            tmpans = reduction(tmp, tmpans);
        }
        else
        {
            MPI_Request request;
            int destination = vtmprank[tmprank - n/(2*i)];
            MPI_Isend(&tmpans, 1, MPI_INT, destination, 0, MPI_COMM_WORLD, &request);
            break;
        }        
    }
    return tmpans;
}

template<class Func, class Reduction>
int parallel_reduce(int l, int r, const parallel_vector& pv, int identity, const Func& func, const Reduction& reduction, int proccess = 0) {
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int proccess_begin = pv.get_index_of_proccess(l);
    int proccess_end = pv.get_index_of_proccess(r-1);
    int ans = identity;
    if(rank >= proccess_begin && rank <= proccess_end) {
        int begin = 0, end = pv.get_portion();
        if(rank == proccess_begin)
            begin = pv.get_index_of_element(l);
        if(rank == proccess_end)
            end = pv.get_index_of_element(r-1)+1;
        ans = func(begin, end, identity);
    }
    else if(rank != proccess)
        return ans;
    return reduce_operation(ans, reduction, proccess_begin, proccess_end, proccess);
}

#endif // __PARALLEL_REDUCE_H__