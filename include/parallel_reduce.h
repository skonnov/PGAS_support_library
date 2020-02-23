#ifndef __PARALLEL_REDUCE_H__
#define __PARALLEL_REDUCE_H__

#include <functional>
#include <mpi.h>
#include <iostream>
#include "parallel_vector.h"
  // std::function<int(int, int)>reduction - ?
  // std::function<int(int, int, const parallel_vector&, int)> func - ?

#define REDUCE_TAG 456

template<class Reduction>
int reduce_operation(int ans, const Reduction& reduction, int process_begin, int process_end, int process = 0) {
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int tmpans = ans;
    int tmprank = 0;
    int t = 1;
    std::vector<int>vtmprank(size+100);
    vtmprank[0] = process;
    for(int i = process_begin; i <= process_end; i++) {
        if(i == process)
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
            MPI_Recv(&tmp, 1, MPI_INT, sender, REDUCE_TAG, MPI_COMM_WORLD, &status);
            tmpans = reduction(tmp, tmpans);
        }
        else
        {
            int destination = vtmprank[tmprank - n/(2*i)];
            tmpans = 0;
            MPI_Send(&tmpans, 1, MPI_INT, destination, REDUCE_TAG, MPI_COMM_WORLD);
            break;
        }        
    }
    return tmpans;
}

template<class Func, class Reduction>
int parallel_reduce(int l, int r, const parallel_vector& pv, int identity, const Func& func, const Reduction& reduction, int process = 0) {
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int process_begin = pv.get_index_of_process(l);
    int process_end = pv.get_index_of_process(r-1);
    int ans = identity;
    if(rank >= process_begin && rank <= process_end) {
        int begin = 0, end = pv.get_portion();
        if(rank == process_begin)
            begin = pv.get_index_of_element(l);
        if(rank == process_end)
            end = pv.get_index_of_element(r-1)+1;
        ans = func(begin, end, identity);
    }
    else if(rank != process)
        return ans;
    return reduce_operation(ans, reduction, process_begin, process_end, process);
}

#endif // __PARALLEL_REDUCE_H__