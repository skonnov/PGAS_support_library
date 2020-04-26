#ifndef __MEMORY_MANAGER_H__
#define __MEMORY_MANAGER_H__

#include <map>
#include <thread>
#include <vector>
#include <queue>
#include <iostream>

#define QUANTUM_SIZE 5

enum mods {
    READ_ONLY,
    READ_WRITE
};

enum tags {
    GET_DATA_FROM_HELPER = 123,
    SEND_DATA_TO_HELPER  = 234,
    SEND_DATA_TO_MASTER_HELPER = 345,
    GET_DATA_FROM_MASTER_HELPER_LOCK = 456,
    SEND_INFO_TO_MASTER_HELPER = 567,
    GET_INFO_FROM_MASTER_HELPER = 678,
    GET_REQUEST_FROM_MASTER_HELPER = 789,
    GET_PERMISSION_FOR_CHANGE_MODE = 890
};

enum operations {
    SET_DATA,
    GET_DATA_RW,
    GET_DATA_R,
    SET_INFO,
    GET_INFO,
    LOCK_READ,
    LOCK_WRITE,
    UNLOCK,
    CHANGE_MODE
};
void worker_helper_thread();
void master_helper_thread();

struct memory_line {  // память для одного parallel_vector
    std::vector<int*>quantums;  // delete bool?
    int logical_size;  // общее число элементов в векторе на всех процессах
    std::vector<std::pair<bool, int>>quantum_owner; // for read_write mode
    std::map<int, std::queue<int>> wait_locks, wait_quantums;  // мапа очередей для процессов, ожидающих разблокировки кванта
    std::vector<int> num_change_mode;
    std::vector<int> quantums_for_lock;
    std::vector<long long> times;
    long long time;
    std::vector<std::vector<int>> owners; // for read_only mode
};

class memory_manager {
    std::vector<memory_line> memory;
    std::thread helper_thr;
    int rank, size;
    int worker_rank, worker_size;
    bool is_read_only_mode;
    int num_of_change_mode_procs;
    int num_of_change_mode;
public:
    void memory_manager_init(int argc, char** argv);  // функция, вызываемая в начале выполнения программы, инициирует вспомогательные потоки
    int get_data(int key, int index_of_element);  // получить элемент по индексу с любого процесса
    void set_data(int key, int index_of_element, int value);  // сохранить значение элемента с любого процесса
    void copy_data(int key_from, int key_to);  // скопировать один memory_line в другой
    int create_object(int number_of_elements);  // создать новый memory_line и занести его в memory
    // void delete_object(int key);
    // int get_size_of_portion(int key);  // получить размер вектора memory из memory_line на данном процессе
    // int get_data_by_index_on_process(int key, int index);  // получить данные по индексу элемента на данном процессе
    // void set_data_by_index_on_process(int key, int index, int value);  // сохранить данные по индексу элемента на данном процессе
    int get_global_index_of_element(int key, int index, int process);  // получить индекс элемента в сквозной нумерации
    int get_number_of_process(int key, int index);  // получить номер процесса, на котором располагается элемент
    int get_number_of_element(int key, int index);  // получить конкретный номер элемента на процессе, на котором он расположен
    int get_quantum_index(int index);  // получить номер кванта по индексу
    void set_lock_read(int key, int quantum_index);  // заблокировать квант
    void set_lock_write(int key, int quantum_index);  // not ready yet
    void unset_lock(int key, int quantum_index);  // разблокировать квант
    void finalize();  // функция, завершающая выполнение программы, останавливает вспомогательные потоки
    // ~memory_manager();
    friend void worker_helper_thread();  // функция, выполняемая вспомогательными потоками процессов-рабочих
    friend void master_helper_thread();  // функция, выполняемая вспомогательным потоком процесса-мастера
    void change_mode(int mode);
};

extern memory_manager mm;

void init(int argc, char** argv);

#endif  // __MEMORY_MANAGER_H__

