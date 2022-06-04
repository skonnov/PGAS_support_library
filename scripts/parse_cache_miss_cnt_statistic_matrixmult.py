from functools import cache
import glob, sys

def parse_output_file(filename):
    f = open(filename)
    results = []

    cache_size = -1
    number_of_elements = 300
    while True:
        line = f.readline()
        if not line:
            break
        line = line.split()
        if line[0] == "cache_size":
            cache_size = int(line[2])
            number_of_elements = 300
            continue
        processes_info = []
        for i in range(0, len(line), 4):
            processes_count = int(line[i][0:-1])
            time = line[i+1]
            minindex = int(line[i+3])
            processes_info.append({'processes_count':processes_count, 'time':time, 'minindex':minindex})
        results.append({'cache_size':cache_size, 'number_of_elements':number_of_elements, 'processes_info':processes_info})
        number_of_elements += 300
    f.close()
    return results

def parse_cache_miss_cnt_statistics(info, filename, number_of_repeats):
    f = open(filename)

    number_of_vectors = 3 # change to determine it by parsing a key index?
    results = []
    info_index = 0
    info_proc_index = 0
    flag = False
    while not flag:
        if flag:
            break
        results_per_vector = []
        for i in range(number_of_repeats):
            for j in range(number_of_vectors):
                print(info_index, info_proc_index, i, j)
                if flag:
                    break

                print(info[info_index]['processes_info'][info_proc_index]['minindex'])
                if i != info[info_index]['processes_info'][info_proc_index]['minindex']:
                    for k in range(6):
                        line = f.readline()
                        if not line:
                            flag = True
                            break
                    continue
                
                print("?")

                line = f.readline()
                if not line:
                    flag = True
                    break

                line = f.readline().split()
                cache_size = int(line[1][0:-1])
                assert(cache_size == info[info_index]['cache_size'])

                number_of_elements_in_vector = int(line[3][0:-1])
                # number_of_processes = int(line[5][0:-1])
                print(info[info_index]['number_of_elements'], number_of_elements_in_vector, info[info_index]['processes_info'][info_proc_index]['processes_count'])
                assert(info[info_index]['number_of_elements'] ** 2 == number_of_elements_in_vector)
                key = int(line[7][0:-1])

                line = f.readline().split()
                cache_miss_cnt_per_process = [int(line[i+2][0:-1]) for i in range(0, len(line), 3)]

                line = f.readline().split()
                cache_evictions_cnt_per_process = [int(line[i+2][0:-1]) for i in range(0, len(line), 3)]

                line = f.readline().split()
                total_cache_miss_cnt = int(line[3][0:-1])
                total_cache_evictions_cnt = int(line[7])

                line = f.readline()

                results_per_vector.append({'key':key, 'number_of_elements_in_vector': number_of_elements_in_vector,
                                         'cache_miss_cnt_per_process': cache_miss_cnt_per_process, 'cache_evictions_cnt_per_process':cache_evictions_cnt_per_process,
                                         'total_cache_miss_cnt':total_cache_miss_cnt, 'total_cache_evictions_cnt':total_cache_evictions_cnt})
            if flag:
                break
        results.append({'cache_size':info[info_index]['cache_size'], 'number_of_elements':info[info_index]['number_of_elements'],
                        'processes_count':info[info_index]['processes_info'][info_proc_index]['processes_count'],
                        'time':info[info_index]['processes_info'][info_proc_index]['time'],
                        'results_per_vector':results_per_vector})
        info_proc_index += 1
        if info_proc_index == len(info[info_index]['processes_info']):
            info_proc_index = 0
            info_index += 1
            if info_index >= len(info):
                flag = True
                break
    f.close()
    
    return results

def print_results_to_file(results_cache_miss_cnt, output_file):
    f = open(output_file, "w")
    f.write("cache_size number_of_elements processes_count time cache_miss_cnt cache_evictions_cnt\n")
    for line in results_cache_miss_cnt:
        total_total_cache_miss_cnt = sum([line['results_per_vector'][i]['total_cache_miss_cnt']
                                            for i in range(len(line['results_per_vector']))])
        total_total_evictions_miss_cnt = sum([line['results_per_vector'][i]['total_cache_evictions_cnt']
                                            for i in range(len(line['results_per_vector']))])
        f.write(str(line['cache_size']) + " " + str(line['number_of_elements']) + " " +
                str(line['processes_count']) + " " + str(line['time']) + " " +
                str(total_total_cache_miss_cnt) + " " + str(total_total_evictions_miss_cnt) + "\n")
    f.close()

if __name__ == "__main__":
    # take the latest output_matrixmult*.txt file
    output_filename_mask = "output_matrixmult*.txt"
    file = sorted(glob.glob(output_filename_mask))[-1]
    if not file:
        print("Can't find file: ", output_filename_mask)
        sys.exit(-1)
    print("took file", file)

    results = parse_output_file(file)
    print(results)

    file_cache_miss_cnt = "cache_miss_cnt.txt"

    number_of_repeats = 3 # change to read it from output_matrixmult?

    results_cache_miss_cnt = parse_cache_miss_cnt_statistics(results, file_cache_miss_cnt, number_of_repeats)

    print(results_cache_miss_cnt)

    output_file = "parsed_cache_miss_cnt.txt"
    print_results_to_file(results_cache_miss_cnt, output_file)