#ifndef __MEMORY_MANAGER_H__
#define __MEMORY_MANAGER_H__

#include <map>
#include <thread>
#include <vector>
#include <queue>
#include <iostream>

#define QUANTUM_SIZE 50

enum tags {
    GET_DATA_FROM_HELPER = 123,
    SEND_DATA_TO_HELPER  = 234,
    SEND_DATA_TO_MASTER_HELPER = 345,
    GET_DATA_FROM_MASTER_HELPER = 456
};

enum operations {
    SET_DATA,
    GET_DATA,
    LOCK,
    UNLOCK
};

struct memory_line {  // память для одного parallel_vector
    std::vector<int> vector;
    std::map<int, std::queue<int>> wait;  // мапа очередей для процессов, ожидающих разблокировки кванта
    int logical_size;  // общее число элементов в векторе на всех процессах
    std::vector<int> quantums;
    int buffer[QUANTUM_SIZE];
    int index_buffer;  // индекс кванта, находящегося в буфере
};

class memory_manager {
    std::vector<memory_line> memory;
    std::thread helper_thr;
    int rank, size;
    int worker_rank, worker_size;
public:
    void memory_manager_init(int argc, char** argv);  // функция, вызываемая в начале выполнения программы, инициирует вспомогательные потоки
    int get_data(int key, int index_of_element);  // получить элемент по индексу с любого процесса
    void set_data(int key, int index_of_element, int value);  // сохранить значение элемента с любого процесса
    void copy_data(int key_from, int key_to);  // скопировать один memory_line в другой
    int create_object(int number_of_elements);  // создать новый memory_line и занести его в memory
    // void delete_object(int key);
    int get_size_of_portion(int key);  // получить размер вектора memory из memory_line на данном процессе
    int get_data_by_index_on_process(int key, int index);  // получить данные по индексу элемента на данном процессе
    void set_data_by_index_on_process(int key, int index, int value);  // сохранить данные по индексу элемента на данном процессе
    int get_logical_index_of_element(int key, int index, int process);  // получить индекс элемента в сквозной нумерации
    std::pair<int, int> get_number_of_process_and_index(int key, int index);  // получить номер процесса,
                                                                              // на котором располагается элемент, и его индекс на этом процессе
    void set_lock(int key, int quantum_index);  // заблокировать квант
    void unset_lock(int key, int quantum_index);  // разблокировать квант
    void finalize();  // функция, завершающая выполнение программы, останавливает вспомогательные потоки
    // ~memory_manager();
    friend void worker_helper_thread();  // функция, выполняемая вспомогательными потоками процессов-рабочих
    friend void master_helper_thread();  // функция, выполняемая вспомогательным потоком процесса-мастера
};

extern memory_manager mm;

void init(int argc, char** argv);

#endif  // __MEMORY_MANAGER_H__