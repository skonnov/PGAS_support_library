import sys
from os.path import exists

import cache_model as cm
# from scipy import optimize
# import bisect as bs

def read_common_statistic(statistic_path: str):
    full_filename = statistic_path + "/common_statistic.txt"
    if (not exists(full_filename)):
        print("Error: common statistic file was not found!")
        exit(1)
    f = open(full_filename)
    line = f.readline().split()
    number_of_processes = int(line[1])
    number_of_vectors = int(line[3])
    vectors_size = []
    caches_size = []
    for i in range(number_of_vectors):
        line = f.readline().split()
        vectors_size.append(int(line[1]))
        caches_size.append(int(line[5]))
    return number_of_processes, number_of_vectors, vectors_size, caches_size

def read_worker_process_info(statistic_path: str, process_number: int, number_of_vectors: int, vectors_size):
    full_filename = statistic_path + "/memory_cache_" + str(process_number) + ".txt"
    if (not exists(full_filename)):
        print("Error: cache statistic file for process " + str(process_number) + " was not found!")
        exit(1)
    f = open(full_filename)
    events = [[] for i in range(number_of_vectors)]
    id = 0
    for _line in f:
        line = _line.split()
        try:
            event = int(line[0])
            if event < 0 or event > 4:
                # print("error on event reading")
                raise ValueError()
            vector_number = int(line[1])
            if vector_number < 0 or vector_number > len(vectors_size):
                # print("error on vector_number reading")
                raise ValueError()
            quantum_number = int(line[2])
            if quantum_number < 0 or quantum_number > vectors_size[vector_number]:
                # print("error on quantum_number reading")
                raise ValueError()
            events[vector_number].append((event, quantum_number))
        except:
            # print("error while reading file " + "memory_cache_" + str(process_number) + ".txt on line: " + str(id + 1))
            pass
        id += 1
    return events

def process(cache_size_arg):
    cache_size = int(cache_size_arg[0])
    number_of_vectors = len(vectors_size)
    cnt_misses = 0
    for process in range(len(events_processes)): # process_worker index
        caches = []
        for i in range(number_of_vectors):
            caches.append(cm.cache(cache_size, vectors_size[i]))
        for v_id, event_vector in enumerate(events_processes[process]):
            for i, event in enumerate(event_vector):
                if event[0] == 0 or event[0] == 4:  # PUT_IN_CACHE or ALREADY_IN_CACHE
                    if (not caches[v_id].is_contain(event[1])):
                        caches[v_id].add(event[1])
                        cnt_misses += 1
                    else:
                        caches[v_id].update(event[1])
                elif event[0] == 1:  # REMOVE_FROM_CACHE
                    pass
                elif event[0] == 2:  # ADD_TO_EXCLUDED
                    if caches[v_id].is_contain(event[1]):
                        caches[v_id].remove(event[1])
                elif event[0] == 3:  # REMOVE_FROM_EXCLUDED
                    pass
                else:
                    assert("unknown event!")
    print(cache_size, ": ", cnt_misses)
    return cnt_misses

def lower_bound(left: int, right: int, best_cnt_misses: int):
    ans = right
    while left <= right:
        c = (left + right) // 2
        cnt_misses = process([c])
        if cnt_misses <= best_cnt_misses:
            right = c - 1
            ans = c
        else:
            left = c + 1
    return ans


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Error: You need to specify path to statistic folder!")
        exit(1)
    statistic_path = sys.argv[1]
    # print("Read statistics from directory: ", statistic_path)
    number_of_processes, number_of_vectors, vectors_size, caches_size = read_common_statistic(statistic_path)
    events_processes = []
    for i in range(1, number_of_processes):
        events_processes.append(read_worker_process_info(statistic_path, i, number_of_vectors, vectors_size))
    cnt_misses = process(caches_size)

    best_value = process([100000])
    print(lower_bound(5, 100000, best_value))
    # bounds = optimize.Bounds(5, 500)
    # print(optimize.minimize(process, x0 = 5, bounds=bounds, method='Nelder-Mead'))
    # res = gp_minimize(process, [(5, 500)], n_calls=20)
    # print(res.x)
