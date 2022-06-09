import glob, sys
import seaborn as sns
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

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
                if flag:
                    break

                if i != info[info_index]['processes_info'][info_proc_index]['minindex']:
                    for k in range(6):
                        line = f.readline()
                        if not line:
                            flag = True
                            break
                    continue

                line = f.readline()
                if not line:
                    flag = True
                    break

                line = f.readline().split()
                cache_size = int(line[1][0:-1])
                assert(cache_size == info[info_index]['cache_size'])

                number_of_elements_in_vector = int(line[3][0:-1])
                # number_of_processes = int(line[5][0:-1])
                # print(info[info_index]['number_of_elements'], number_of_elements_in_vector, info[info_index]['processes_info'][info_proc_index]['processes_count'])
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
    # f.write("cache_size number_of_elements processes_count time cache_miss_cnt cache_evictions_cnt\n")

    cache_size = -1
    number_of_elements = -1

    f.write("time\n")
    f.write("number_of_elements 2 5 10 17\n")

    for line in results_cache_miss_cnt:
        if line['cache_size'] != cache_size:
            cache_size = line['cache_size']
            f.write("\n\ncache_size: " + str(line['cache_size']) + "\n")
        if line['number_of_elements'] != number_of_elements:
            number_of_elements = line['number_of_elements']
            f.write('\n' + str(line['number_of_elements']) + " ")
        f.write(str(line['time']) + " ")

    f.write('\n~~~~~~~~~~\n')

    cache_size = -1
    number_of_elements = -1

    f.write("cache_misses\n")
    f.write("number_of_elements 2 5 10 17\n")

    for line in results_cache_miss_cnt:
        if line['cache_size'] != cache_size:
            cache_size = line['cache_size']
            f.write("\n\ncache_size: " + str(line['cache_size']) + "\n\n")
        if line['number_of_elements'] != number_of_elements:
            number_of_elements = line['number_of_elements']
            f.write('\n' + str(line['number_of_elements']) + " ")
        total_total_cache_miss_cnt = sum([line['results_per_vector'][i]['total_cache_miss_cnt']
                                            for i in range(len(line['results_per_vector']))])
        f.write(str(total_total_cache_miss_cnt) + " ")

    f.write('\n~~~~~~~~~~\n')

    cache_size = -1
    number_of_elements = -1

    f.write("cache_evictions\n")
    f.write("number_of_elements 2 5 10 17\n")

    for line in results_cache_miss_cnt:
        if line['cache_size'] != cache_size:
            cache_size = line['cache_size']
            f.write("\n\ncache_size: " + str(line['cache_size']) + "\n\n")
        if line['number_of_elements'] != number_of_elements:
            number_of_elements = line['number_of_elements']
            f.write('\n' + str(line['number_of_elements']) + " ")
        total_total_evictions_miss_cnt = sum([line['results_per_vector'][i]['total_cache_miss_cnt']
                                            for i in range(len(line['results_per_vector']))])
        f.write(str(total_total_evictions_miss_cnt) + " ")

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
    # print(results)

    file_cache_miss_cnt = "cache_miss_cnt.txt"

    number_of_repeats = 3 # change to read it from output_matrixmult?

    results_cache_miss_cnt = parse_cache_miss_cnt_statistics(results, file_cache_miss_cnt, number_of_repeats)

    # print(results_cache_miss_cnt)

    output_file = "parsed_cache_miss_cnt.txt"
    print_results_to_file(results_cache_miss_cnt, output_file)

    times = {}
    number_of_elements = []
    for line in results_cache_miss_cnt:
        if line['cache_size'] not in times:
            times[line['cache_size']] = {}
        if line['processes_count'] not in times[line['cache_size']]:
            times[line['cache_size']][line['processes_count']] = []

        # change it?
        if line['number_of_elements'] not in number_of_elements:
            number_of_elements.append(line['number_of_elements'])

        times[line['cache_size']][line['processes_count']].append(float(line['time']))

    cache_evictions = {}
    for line in results_cache_miss_cnt:
        if line['cache_size'] not in cache_evictions:
            cache_evictions[line['cache_size']] = {}
        if line['processes_count'] not in cache_evictions[line['cache_size']]:
            cache_evictions[line['cache_size']][line['processes_count']] = []
        total_total_evictions_miss_cnt = sum([line['results_per_vector'][i]['total_cache_evictions_cnt']
                                    for i in range(len(line['results_per_vector']))])
        cache_evictions[line['cache_size']][line['processes_count']].append(float(total_total_evictions_miss_cnt))

    accelerations = times.copy()
    for cache_size in times:
        accelerations[cache_size] = times[cache_size].copy()
        for processes_count in times[cache_size]:
            accelerations[cache_size][processes_count] = times[cache_size][processes_count].copy()
            for i in range(len(times[cache_size][processes_count])):
                accelerations[cache_size][processes_count][i] = times[cache_size][2][i] / times[cache_size][processes_count][i]

    data_times = {cache_size:pd.DataFrame(accelerations[cache_size], index=number_of_elements) for cache_size in times}

    data_cache_evictions = {cache_size:pd.DataFrame(cache_evictions[cache_size], index=number_of_elements) for cache_size in cache_evictions}


    fig, axes = plt.subplots(len(times) + 1, 2, figsize=(12, 12), gridspec_kw={"height_ratios":[0.02,1,1,1,1]})
    fig.tight_layout(pad=3.0, h_pad=3.0, w_pad=1.0)

    axes[0, 0].axis("off")
    axes[0, 1].axis("off")
    axes[0, 0].set_title("Ускорение", fontsize=14)
    axes[0, 1].set_title("Число вытеснений из кеша", fontsize=14)

    for i, cache_size in enumerate(data_times):
        acceleration = sns.heatmap(ax=axes[i+1, 0], data=data_times[cache_size], cmap='coolwarm', vmin = 0., vmax = 7.0, cbar_kws={'label': 'ускорение, раз'})
        acceleration.set_title("размер кеша = " + str(cache_size), fontsize=14)
        acceleration.set_xlabel("число процессов",fontsize=12)
        acceleration.set_ylabel("число элементов",fontsize=12)

        cache_evictions = sns.heatmap(ax=axes[i+1, 1], data=data_cache_evictions[cache_size], cmap='coolwarm', cbar_kws={'label': 'к-во вытеснений из кеша'})
        cache_evictions.set_title("размер кеша = " + str(cache_size), fontsize=14)
        cache_evictions.set_xlabel("число процессов",fontsize=12)
        cache_evictions.set_ylabel("число элементов",fontsize=12)

    fig, axes = plt.subplots(len(times) // 2, 2, figsize=(15, 10))
    fig.tight_layout(pad=5.0)
    for i, cache_size in enumerate(data_times):
        lines = data_times[cache_size].transpose().plot.line(ax=axes[i // 2, i % 2], marker='o')
        lines.set_title("размер кеша = " + str(cache_size), fontsize=14)
        lines.set_xticks([2, 5, 10, 17], fontsize=10)
        lines.set_yticks(np.arange(0, 7, step=0.5),fontsize=10)
        lines.set_xlabel("число процессов",fontsize=14)
        lines.set_ylabel("ускорение",fontsize=14)

    plt.show()
