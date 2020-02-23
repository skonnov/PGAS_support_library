// #include <iostream>
// #include <mpi.h>
// #include "parallel_vector.h"
// #include "parallel_reduce.h"
// #include "parallel_for.h"


class Func {
public: 
    parallel_vector* a;
    std::vector<int>* b;
    Func(parallel_vector& pv, std::vector<int>&v):a(&pv), b(&v) {}
    int operator()(int l, int r, int identity)  const {
        int ans = identity;
        for(int i = l; i < r; i++) {
            int rank;
            MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            ans+=a->get_elem_proc(i)*(*b)[a->get_reverse_index_of_element(i, rank)%(b->size())];
        }
        return ans;
    }  
};

class Reduction {
    int a, b;
public:
    Reduction(): a(0), b(0) {}
    Reduction(int aa, int bb): a(aa), b(bb) {}
    int operator()(int a, int b) const {
        return a+b;
    }
};


int main(int argc, char** argv) { // b*a
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int n = atoi(argv[1]);
    int m = atoi(argv[2]);
    parallel_vector a(n*m);
    //parallel_vector b(n);
    std::vector<int>b(n);
    parallel_vector ans(m);
    for(int i = 0; i < n*m; i++) {
        a.set_elem(i, i);
    }
    for(int i = 0; i < n; i++) {
        // b.set_elem(i, i);
        b[i] = i;
    }
    double t1 = MPI_Wtime();
    for(int i = 0; i < m; i++) {
        int proccess = ans.get_index_of_proccess(i);
        int anss = parallel_reduce(i*n, (i+1)*n, a, 0, Func(a, b), Reduction(), proccess);
        ans.set_elem(i, anss);
    }
    double t2 = MPI_Wtime();
    if(rank == 0)
        std::cout<<t2-t1;
    MPI_Finalize();
    return 0;
}


class Func {  // функтор, собирающий произведение матрицы на элементы вектора на одном процессе
    parallel_vector a;
    std::vector<int>b;
public:
    int operator()(int l, int r, int identity)  const { // l, r - границы, identity - начальное значение
        int ans = identity;
        for(int i = l; i < r; i++)
            ans+=a[i]*b[index_of_element_on_process%b.size()]
        return ans;
    }  
};

class Reduction {  // функтор, объединяющий значения переменных на разных процессах
    int a, b;
public:
    int operator()(int a, int b) const {
        return a+b;
    }
};

parallel_vector a(n*m);
std::vector<int>b(n);
parallel_vector ans(m);
int identity = 0;
for(int i = 0; i < m; i++) {
    int process = ans.get_index_of_process(i); // номер процесса, на котором сохраняется ответ
    int ans[i] = parallel_reduce(i*n, (i+1)*n, a, identity, Func(a, b), Reduction(), proccess);
}