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

// int dijkstra(const std::vector<std::vector<std::pair<int, int>>>& v, parallel_vector<int>& d, int n, int begin, int end, int quantum_size = DEFAULT_QUANTUM_SIZE) {
//     int size = memory_manager::get_MPI_size();

//     int count = 2;
//     int blocklens[] = {1, 1};
//     MPI_Aint indices[] = {
//         (MPI_Aint)offsetof(pair_reduce, first),
//         (MPI_Aint)offsetof(pair_reduce, second)
//     };
//     MPI_Datatype types[] = { MPI_INT, MPI_INT };
//     MPI_Datatype type = create_mpi_type<pair_reduce>(2, blocklens, indices, types);

//     parallel_priority_queue<pair> pq({0, 0}, int(v.size() + quantum_size - 1) / quantum_size, quantum_size); // num_of_quantums_proc - ?
//     int k = 0;
//     int start = begin;
//     while (k < n) {
//         // I need some kind of condition variables for processes?
//         pair cur;
//         int size = pq.get_size();
//         for (int i = 1; i < size; i++) {
//             cur = pq.get_max(i);
//             pq.remove_max();
//         }

//         int cur_v = cur.second;
//         int cur_d = -cur.first;

//         int cur_vd = d.get_elem(cur_v);

//         if(cur_d > cur_vd)
//             continue;

//         for (int i = 0; i < v[i].size(); i++) {
//             int to = v[cur_v][i].first;
//             int len = v[cur_v][i].second;

//             d.set_lock(to);
//             if (cur_vd + len < d.get_elem(to)) {
//                 d.set_elem(to, cur_vd + len);
//                 // pq.insert({-cur_vd-len, to}, worker_rank);
//             }
//             d.unset_lock(to);
//         }
//     }
//     return 0;
// }

int main(int argc, char ** argv) {
//     memory_manager::memory_manager_init(argc, argv);

//     int n, m;  // n - число вершин, m - число рёбер


//     memory_manager::finalize();
    return 0;
}