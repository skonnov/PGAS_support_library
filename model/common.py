from os.path import exists

def parse_argv(argv):
    ret_argv = {}
    for i in range(len(argv)):
        if argv[i].startswith('-') and i < len(argv) - 1:
            ret_argv[argv[i][1:]] = argv[i + 1]
            i += 1
    return ret_argv

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
