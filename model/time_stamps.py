import sys

import common as common

def show_usage():
    print("usage: python time_stamps.py <-path path/to/statistic/folder>")

if __name__ == "__main__":
    parsed_argv = common.parse_argv(sys.argv)

    if "path" not in parsed_argv:
        print("Error: You need to specify path to statistic folder!")
        show_usage()
        exit(1)

    statistic_path = parsed_argv["path"]

    number_of_processes, number_of_vectors, vectors_size, caches_size = common.read_common_statistic(statistic_path)

    quantums_info = [[[] for j in range(vectors_size[i])] for i in range(number_of_vectors)]
