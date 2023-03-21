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

def process(cache_size_arg, put_in_cache_weight: int, already_in_cache_weight: int):
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
                        cnt_misses += put_in_cache_weight
                    else:
                        cnt_misses += already_in_cache_weight
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
    # print(cache_size, ": ", cnt_misses)
    return cnt_misses

def lower_bound(left: int, right: int, best_cnt_misses: int, put_in_cache_weight: int, already_in_cache_weight: int):
    ans = right
    while left <= right:
        c = (left + right) // 2
        cnt_misses = process([c], put_in_cache_weight, already_in_cache_weight)
        if cnt_misses <= best_cnt_misses:
            right = c - 1
            ans = c
        else:
            left = c + 1
    return ans

def parse_argv(argv):
    ret_argv = {}
    for i in range(len(argv)):
        if argv[i].startswith('-') and i < len(argv) - 1:
            ret_argv[argv[i][1:]] = argv[i + 1]
            i += 1
    return ret_argv

def show_usage():
    print("usage: python cache_statistic_model.py <-path path/to/statistic/folder> [-pw put_in_cache_weight (default = 1)] [-aw already_in_cache_weight (default = 0)]")

if __name__ == "__main__":
    parsed_argv = parse_argv(sys.argv)
    if "path" not in parsed_argv:
        print("Error: You need to specify path to statistic folder!")
        show_usage()
        exit(1)
    statistic_path = parsed_argv["path"]
    put_in_cache_weight = 1
    already_in_cache_weight = 0
    if "pw" in parsed_argv:
        put_in_cache_weight = int(parsed_argv["pw"])
        assert(put_in_cache_weight >= 0)

    if "aw" in parsed_argv:
        already_in_cache_weight = int(parsed_argv["aw"])
        assert(already_in_cache_weight >= 0)

    # print("Read statistics from directory: ", statistic_path)
    number_of_processes, number_of_vectors, vectors_size, caches_size = read_common_statistic(statistic_path)
    events_processes = []
    for i in range(1, number_of_processes):
        events_processes.append(read_worker_process_info(statistic_path, i, number_of_vectors, vectors_size))
    cnt_misses = process(caches_size, put_in_cache_weight, already_in_cache_weight)

    best_value = process([100000], put_in_cache_weight, already_in_cache_weight)
    print(lower_bound(5, 100000, best_value, put_in_cache_weight, already_in_cache_weight))
    # bounds = optimize.Bounds(5, 500)
    # print(optimize.minimize(process, x0 = 5, bounds=bounds, method='Nelder-Mead'))
    # res = gp_minimize(process, [(5, 500)], n_calls=20)
    # print(res.x)
