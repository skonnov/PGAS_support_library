#include <iostream>
#include "parallel_vector.h"
#include "memory_manager.h"
#include "parallel_priority_queue.h"
#include <climits>
#include <ctime>
#include <algorithm>
#include <random>
#include <queue>

struct pair
{
    int first;
    int second;

    bool operator<(const pair& p) const {
        if (first == p.first)
            return second < p.second;
        return first < p.first;
    }
};

std::ostream &operator<<(std::ostream &out, pair const &m) {
    return out << "("<<m.first<<" "<<m.second<<")";
}

int dijkstra(const std::vector<std::vector<std::pair<int, int>>>& v, parallel_vector<int>& d, int n, int begin, int end, int quantum_size = DEFAULT_QUANTUM_SIZE, int to_worker_rank = 0) {
    int worker_rank = memory_manager::get_MPI_rank() - 1;
    int worker_size = memory_manager::get_MPI_size() - 1;
    int count = 2;
    int blocklens[] = {1, 1};
    MPI_Aint indices[] = {
        (MPI_Aint)offsetof(pair, first),
        (MPI_Aint)offsetof(pair, second)
    };
    MPI_Datatype types[] = { MPI_INT, MPI_INT };


    parallel_priority_queue<pair> pq(count, blocklens, indices, types,
                                     {INT_MIN, INT_MIN}, int(v.size() * v.size() + quantum_size - 1) / quantum_size, quantum_size); // num_of_quantums_proc - ?
    if (worker_rank < 0)
        return 0;
    int k = 0;
    int start = begin;
    pq.insert({0, begin});
    d.set_elem(begin, 0);
    while (k < n) {
        pair cur_pair = pq.get_and_remove_max();
        int cur = cur_pair.second;
        int curlen = -cur_pair.first;

        if (curlen == INT_MIN)
            return -1;

        int cur_d = d.get_elem(cur);
        if (curlen > cur_d)
            continue;
        k++;

        int portion_begin = 0, portion_end = 0;
        int cur_size = static_cast<int>(v[cur].size());
        if (worker_rank < cur_size % worker_size) {
            portion_begin = (cur_size / worker_size + 1) * worker_rank;
            portion_end = portion_begin + (cur_size / worker_size + 1);
        } else {
            portion_begin = (cur_size / worker_size + 1) * (cur_size % worker_size)
                    + (cur_size / worker_size) * (worker_rank - (cur_size % worker_size));
            portion_end = portion_begin + (cur_size / worker_size);
        }

        for (int i = portion_begin; i < portion_end; i++) {
            int to = v[cur][i].first, len = v[cur][i].second;
            int to_d = d.get_elem(to);
            if (cur_d + len < to_d) {
                d.set_elem(to, cur_d + len);
                pq.insert_local({-(cur_d + len), to});
            }
        }
    }
    if (worker_rank == to_worker_rank)
        return d.get_elem(end);
    else
        return 0;
}

int dijkstra_seq(const std::vector<std::vector<std::pair<int, int>>>& v, int begin, int end) {
    std::vector<int> d(v.size(), INT_MAX);
    d[begin] = 0;

    std::priority_queue<std::pair<int, int>> q;
    q.push({0, begin});
    while (!q.empty()) {
        int cur = q.top().second, cur_d = -q.top().first;
        q.pop();
        if (cur_d > d[cur])
            continue;
        for (auto x: v[cur]) {
            int to = x.first;
            int len = x.second;
            if (d[cur] + len < d[to]) {
                d[to] = d[cur] + len;
                q.push({-d[to], to});
            }
        }
    }
    return d[end];
}

std::vector<std::vector<std::pair<int,int>>> generate_graph(int n, int m, int min_size, int max_size, int seed) {
    std::vector<std::pair<int, int>> all;
    std::vector<std::vector<std::pair<int, int>>> v(n);

    for(int i = 0; i < n; i++)
        for (int j = i+1; j < n; j++)
            all.push_back({i, j});

    int rank = memory_manager::get_MPI_rank();
    std::shuffle(std::begin(all), std::end(all), std::default_random_engine(seed));

    std::mt19937 mt(seed);
    std::uniform_int_distribution<int> rand(1, max_size);
    for (int i = 0; i < m; i++) {
        int size = rand(mt);
        v[all[i].first].push_back({ all[i].second, size });
        v[all[i].second].push_back({ all[i].first, size });
    }

    for (int i = 0; i < n; i++) {
        std::sort(v[i].begin(), v[i].end());
    }
    return v;
}

void print_graph(const std::vector<std::vector<std::pair<int, int>>>& v) {
    for (int i = 0; i < v.size(); i++) {
        std::cout<<i<<": ";
        for (int j = 0; j < v[i].size(); j++) {
            std::cout<<"("<<v[i][j].first<<", "<<v[i][j].second<<")";
            if (j < v[i].size() - 1)
                std::cout<<", ";
        }
        std::cout<<std::endl;
    }
}

static void show_usage() {
    if (memory_manager::get_MPI_rank() == 1)
        std::cerr << "Usage: mpiexec <-n num_of_processes> dijkstra.exe <-v num_of_vertices> [-e num_of_edges] [-s seed] [-min min_edge_size] [-max max_edge_size]"<<std::endl;
}

int get_args(int argc, char** argv, int& n, int&m, int& seed, int& min_size, int& max_size) {
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "-v") {
            if (i+1 < argc) {
                n = atoi(argv[++i]);
            } else {
                return -1;
            }
        }
        if (std::string(argv[i]) == "-e") {
            if (i+1 < argc) {
                m = atoi(argv[++i]);
            } else {
                return -1;
            }
        }

        if (std::string(argv[i]) == "-s") {
            if (i+1 < argc) {
                seed = atoi(argv[++i]);
            } else {
                return -1;
            }
        }

        if (std::string(argv[i]) == "-min") {
            if (i+1 < argc) {
                min_size = atoi(argv[++i]);
            } else {
                return -1;
            }
        }

        if (std::string(argv[i]) == "-max") {
            if (i+1 < argc) {
                max_size = atoi(argv[++i]);
            } else {
                return -1;
            }
        }
    }

    if (min_size > max_size) {
        if (memory_manager::get_MPI_rank() == 1)
            std::cerr<<"min size of edge must be less then max size!"<<std::endl;
        return -1;
    }

    if (n == -1) {
        if (memory_manager::get_MPI_rank() == 1)
            std::cerr<<"You need to define num of vertices!"<<std::endl;
        return -1;
    }

    if (m == -1) {
        m = n * (n-1) / 2;
    }

    if (n < 0 || m < 0) {
        if (memory_manager::get_MPI_rank() == 1)
            std::cerr<<"vertices and edges must be positive!"<<std::endl;
        return -1;
    }

    if (n * (n-1) / 2 < m) {
        if (memory_manager::get_MPI_rank() == 1)
            std::cerr<<"num of edges must be equal or less then vertices * (vertices - 1) / 2!"<<std::endl;
        return -1;
    }

    return 0;
}

int main(int argc, char** argv) {
    memory_manager::memory_manager_init(argc, argv);
    int n = -1, m = -1;  // n - число вершин, m - число рёбер
    int seed = 0;
    int min_size = 1;
    int max_size = 50000;

    int res = get_args(argc, argv, n, m, seed, min_size, max_size);
    if (res == -1) {
        show_usage();
        memory_manager::finalize();
        return 0;
    }

    parallel_vector<int> d(n);

    int rank = memory_manager::get_MPI_rank();
    if (rank == 1)
        for (int i = 0; i < n; i++) {
            d.set_elem(i, INT_MAX);
        }
    MPI_Barrier(MPI_COMM_WORLD);

    std::vector<std::vector<std::pair<int,int>>> v = generate_graph(n, m, min_size, max_size, seed);
    double t1 = MPI_Wtime();
    int ans = dijkstra(v, d, n, 0, n-1, DEFAULT_QUANTUM_SIZE);
    double t2 = MPI_Wtime();
    if (rank == 1) {
        if (ans != dijkstra_seq(v, 0, n-1)) {
            std::cout<<"ALYARMA!"<<std::endl;
        }
        std::cout<<t2-t1<<std::endl;

    }

    memory_manager::finalize();
    return 0;
}
