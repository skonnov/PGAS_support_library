#ifndef __PARALLEL_FOR_H__
#define __PARALLEL_FOR_H__

#include <functional>
#include "parallel_vector.h"
#include "mpi.h"

class parallel_for {
    int rank, size;
    int function_number;
public:
    parallel_for();
    void operator()(int l, int r, parallel_vector& pv, std::function<int(int)> f);
};

#endif // __PARALLEL_FOR_H__
