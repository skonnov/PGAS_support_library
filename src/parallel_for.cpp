#include "parallel_for.h"
#include <iostream>
parallel_for::parallel_for() {
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    // ans = parallel_vector(size);
    function_number = -1;
}

/*void parallel_for::operator()(const parallel_vector& pv, int num) {
    int portion = pv.get_portion();
    function_number = num;
    ans.set_elem_proc(0, 0);
    for(int i = 0; i < portion; i++) {
        if(num == PARALLEL_SUM)
        {
            ans.set_elem_proc(0, ans.get_elem_proc(0) + pv.get_elem_proc(i));
        }
    }
}*/

void parallel_for::operator()(int l, int r, parallel_vector& pv, std::function<int(int)> f) {
    if(l > r)
        return;
    int proccess_begin = pv.get_index_of_process(l);
    int proccess_end = pv.get_index_of_process(r);
    if(rank >= proccess_begin && rank <= proccess_end) {
        int begin = 0, end = pv.get_portion();
        if(rank == proccess_begin)
            begin = pv.get_index_of_element(l);
        if(rank == proccess_end)
            end = pv.get_index_of_element(r);
        for(int i = begin; i < end; i++)
            pv.set_elem_proc(i, f(pv.get_elem_proc(i)));
    }
}


/*int parallel_for::get_ans(int proccess) {
    int tmp = ans.get_elem_proc(0);
    int tmp1 = 0;
    if(function_number == PARALLEL_SUM)
        MPI_Reduce(&tmp, &tmp1, 1, MPI_INT, MPI_SUM, proccess, MPI_COMM_WORLD);
    if(rank == proccess)
        return tmp1;
    return tmp;
}*/
