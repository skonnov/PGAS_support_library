#include <iostream>
#include "parallel_vector.h"
#include "memory_manager.h"
#include "parallel_priority_queue.h"
#include <climits>

struct pair
{
    int first;
    int second;

    bool operator<(const pair& p) const {
        if (first == p.first)
            return second < p.second;
        return second;
    }
};

int dijkstra(const std::vector<std::vector<std::pair<int, int>>>& v, parallel_vector<int>& d, int n, int begin, int end, int quantum_size = DEFAULT_QUANTUM_SIZE) {
    int worker_rank = memory_manager::get_MPI_rank() - 1;
    int worker_size = memory_manager::get_MPI_size() - 1;

    int count = 2;
    int blocklens[] = {1, 1};
    MPI_Aint indices[] = {
        (MPI_Aint)offsetof(pair_reduce, first),
        (MPI_Aint)offsetof(pair_reduce, second)
    };
    MPI_Datatype types[] = { MPI_INT, MPI_INT };
    MPI_Datatype type = create_mpi_type<pair_reduce>(2, blocklens, indices, types);

    parallel_priority_queue<pair> pq({0, 0}, int(v.size() + quantum_size - 1) / quantum_size, quantum_size); // num_of_quantums_proc - ?
    int k = 0;
    int start = begin;
    pq.insert({0, begin});
    while (k < n) {
        pair cur = pq.get_max();
        pq.remove_max(); // unite this functions?
        
        int cur_d = d.get_elem(cur.second);
        if (cur.first > cur_d)
            continue;
        k++;

        int portion_begin = 0, portion_end = 0;
        int cur_size = v[cur.second].size();
        if (worker_rank < cur_size % worker_size) {
            portion_begin = (cur_size / worker_size + 1) * worker_rank;
            portion_end = portion_begin + (cur_size / worker_size + 1);
        } else {
            portion_begin = (cur_size / worker_size + 1) * (cur_size % worker_size) + (cur_size / worker_size) * (worker_rank - (cur_size % worker_size));
            portion_end = portion_begin + (cur_size / worker_size);
        }

        for (int i = portion_begin; i < portion_end; i++) {
            int to = v[cur.second][i].first, len = v[cur.second][i].second;
            int to_d = d.get_elem(to);
            if (cur_d + len < to_d) {
                d.set_elem(to, cur_d + len);
                pq.insert_local({-(cur_d + len), to});
            }
        }
    }
    return d.get_elem(end);
}

int main(int argc, char ** argv) {
    memory_manager::memory_manager_init(argc, argv);

    int n, m;  // n - число вершин, m - число рёбер


    memory_manager::finalize();
    return 0;
}