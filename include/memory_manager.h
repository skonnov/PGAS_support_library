#ifndef __MEMORY_MANAGER_H__
#define __MEMORY_MANAGER_H__

#include <map>
#include <thread>
#include <vector>
#include <iostream>

#define QUANTUM_SIZE 50

struct memory_line {
    std::vector<int> vector;
    int logical_size;
    std::vector<int> quantums;
};

class memory_manager {
    std::vector<memory_line> memory;
    std::thread helper_thr;
    int rank, size;
    int worker_rank, worker_size;
public:
    void memory_manager_init(int argc, char** argv);
    int get_data(int key, int index_of_element); 
    void set_data(int key, int index_of_element, int value);
    void copy_data(int key_from, int key_to);
    int create_object(int number_of_elements);
    // void delete_object(int key);
    int get_size_of_portion(int key);
    int get_data_by_index_on_process(int key, int index);
    void set_data_by_index_on_process(int key, int index, int value);
    int get_logical_index_of_element(int key, int index, int process);
    std::pair<int, int> get_number_of_process_and_index(int key, int index);
    void finalize();
    ~memory_manager();
    friend void helper_thread();
};

extern memory_manager mm;

void init(int argc, char** argv);

#endif  // __MEMORY_MANAGER_H__