#ifndef __MEMORY_MANAGER_H__
#define __MEMORY_MANAGER_H__

#include <map>
#include <thread>
#include <vector>
#include <iostream>
class memory_manager {
    std::vector<std::vector<int>>memory;
    std::thread helper_thr;
    int rank, size;
public:
    void memory_manager_init(int argc, char** argv);
    int get_data(int key, int index_of_element); 
    void set_data(int key, int index_of_element, int value);
    int create_object(int number_of_elements);
    // void delete_object(int key);
    std::vector<int>& get_memory(int size);
    // int get_number_by_pointer(int* pointer);
    // int* get_pointer_by_number(int number);
    std::pair<int, int> get_number_of_process_and_index(int key, int index);
    void finalize();
    ~memory_manager();
    friend void helper_thread();
};

extern memory_manager mm;

void init(int argc, char** argv);

#endif  // __MEMORY_MANAGER_H__