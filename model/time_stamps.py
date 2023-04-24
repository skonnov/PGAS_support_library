import sys

from enum import Enum
import common as common
from os.path import exists

class op(Enum):
    PUSH = 0
    POP = 1
    CHANGE_MODE = 2

class mode(Enum):
    READ_WRITE = 0
    READ_ONLY = 1

def show_usage():
    print("usage: python time_stamps.py <-path path/to/statistic/folder>")

# info for each quantum step:
# add/delete:
# <op> <key> <quantum_index> <process> <time>
# change mode:
# <op> <key> <quantum_index_l> <quantum_index_r> <process> <time>
def read_quantums_schedule_statistic(statistic_path: str):
    full_filename = statistic_path + "/" + "quantums_schedule_raw.txt"
    if (not exists(full_filename)):
        print("Error: quantums schedule file was not found!")
        exit(1)

    out = []
    f = open(full_filename)

    for _line in f:
        line = _line.split()
        operation = op(int(line[0]))
        key = int(line[1])
        time = float(line[4])
        if operation == op.PUSH or operation == op.POP:
            quantum = int(line[2])
            process = int(line[3])
            out.append({"op": operation, "key": key, "quantum_index": quantum, "process": process, "time": time})
        else:  # op.CHANGE_MODE:
            quantum_l = int(line[2])
            quantum_r = int(line[3])
            out.append({"op": operation, "key": key, "quantum_index_l": quantum_l, "quantum_index_r": quantum_r, "time": time})

    f.close()
    return out


def update_timestamps(timestamps, quantums_processes):
    for vector_id in range(len(quantums_processes)):
        for quantum_id in range(len(quantums_processes[vector_id])):
            for process in quantums_processes[vector_id][quantum_id]:
                timestamps[vector_id][quantum_id][process] += 1

# timestamps - array of counters for each process in each quantum in each vector
# e.g. timestamps[5][6][3] = 6: counter for vector #5, quantum #6, process #3 = 6
def collect_time_stamps(time_step: float, number_of_processes, number_of_vectors, vectors_size, quantums_schedule_statistic):
    timestamps = [[[0 for process_number in range(number_of_processes)] for quantum_id in range(vectors_size[vector_id])] for vector_id in range(number_of_vectors)]
    current_time_target = time_step

    # contains current process numbers for each quantum and each vector
    quantums_processes = [[[] for quantum_id in range(vectors_size[vector_id])] for vector_id in range(number_of_vectors)]

    # contains current quantum mode and bool variable to check if quantum mode was changed
    quantums_mode = [[[mode.READ_WRITE, False] for quantum_id in range(vectors_size[vector_id])] for vector_id in range(number_of_vectors)]

    for line in quantums_schedule_statistic:
        while line["time"] > current_time_target:
            update_timestamps(timestamps, quantums_processes)
            current_time_target += time_step

        key = line["key"]
        if line["op"] == op.PUSH or line["op"] == op.POP:
            quantum_id = line["quantum_index"]
            process = line["process"]
            if quantums_mode[key][quantum_id][1] == True:  # mode was changed
                if quantums_mode[key][quantum_id][key] == mode.READ_WRITE:
                    quantums_processes[key][quantum_id].clear()
                quantums_mode[key][quantum_id][1] = False

            if quantums_mode[key][quantum_id][0] == mode.READ_WRITE:
                if len(quantums_processes[key][quantum_id]) == 0:
                    quantums_processes[key][quantum_id].append(-1)
                quantums_processes[key][quantum_id][0] = process
            else:  # quantum mode = READ_ONLY
                if line["op"] == op.PUSH:
                    quantums_processes[key][quantum_id].append(process)
                else:
                    quantums_processes[key][quantum_id].remove(process)
        else:  # line["op"] == op.CHANGE_MODE:
            for quantum_id in range(line["quantum_index_l"], line["quantum_index_r"]):
                if quantums_mode[key][quantum_id][0] == mode.READ_ONLY:
                    quantums_mode[key][quantum_id][0] = mode.READ_WRITE
                else:
                    quantums_mode[key][quantum_id][0] = mode.READ_ONLY
                quantums_mode[key][quantum_id][1] = True
    return timestamps


if __name__ == "__main__":
    parsed_argv = common.parse_argv(sys.argv)

    if "path" not in parsed_argv:
        print("Error: You need to specify path to statistic folder!")
        show_usage()
        exit(1)

    time_step = 0.001  # in seconds

    if "time_step" in parsed_argv:
        time_step = float(parsed_argv["time_step"])

    statistic_path = parsed_argv["path"]

    number_of_processes, number_of_vectors, vectors_size, caches_size = common.read_common_statistic(statistic_path)

    quantums_schedule_statistic = read_quantums_schedule_statistic(statistic_path)

    timestamps = collect_time_stamps(time_step, number_of_processes, number_of_vectors, vectors_size, quantums_schedule_statistic)
    for key in range(number_of_vectors):
        print("vector ", key, ":", sep="")
        for quantum_number in range(len(timestamps[key])):
            print("    quantum ", quantum_number, ":", sep="")
            print("        ", end="")
            for process in range(len(timestamps[key][quantum_number])):
                print(timestamps[key][quantum_number][process], end=" ")
            print("")
