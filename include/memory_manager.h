#ifndef __MEMORY_MANAGER_H__
#define __MEMORY_MANAGER_H__

#include <thread>
#include <vector>
#include <deque>
#include <mutex>
#include <iostream>
#include <fstream>
#include <cassert>
#include <string>
#include <set>
#include <mpi.h>
#include "common.h"
#include "memory_allocator.h"
#include "queue_quantums.h"

void worker_helper_thread();
void master_helper_thread();

struct memory_line_common {
    std::vector<int> mode;  // режим работы с квантом
    std::vector<bool> is_mode_changed;
    std::vector<int> num_of_change_mode_procs;
    int logical_size;  // общее число элементов в векторе на всех процессах
    int quantum_size;
    virtual ~memory_line_common() {}
};

struct memory_line_worker
    : public memory_line_common {
    std::vector<int*> quantums;  // вектор указателей на кванты
    memory_allocator allocator;
    std::vector<std::mutex*> mutexes;  // мьютексы на каждый квант, нужны, чтобы предотвратить одновременный доступ
                                       // к кванту с разных потоков в режиме READ_WRITE
};

struct memory_line_master
    : public memory_line_common {
    std::vector<bool> quantum_ready;  // готов ли квант для передачи
    queue_quantums wait_locks;  // мапа очередей для процессов, ожидающих разблокировки кванта, заблокированных через set_lock
    queue_quantums wait_quantums;  // мапа очередей для процессов, ожидающих разблокировки кванта, заблокированных процессом-мастером
    std::vector<int> quantums_for_lock;  // вектор для определения номеров процессов, блокирующих кванты
    std::vector<std::deque<int>> owners; // для read_only mode, номера процессов, хранящих у себя квант

};

class memory_manager {
    static std::vector<memory_line_common*> memory;  // структура-хранилище памяти и вспомогательной информации
    static std::thread helper_thr;  // вспомогательный поток
    static int rank, size;  // ранг процесса в MPI и число процессов
    static int worker_rank, worker_size;  // worker_rank = rank-1, worker_size = size-1
    static int proc_count_ready;
    static MPI_File fh;
    static MPI_Comm workers_comm;
public:
    static void memory_manager_init(int argc, char** argv, std::string error_helper = "");  // функция, вызываемая в начале выполнения программы, инициирует вспомогательные потоки
    static int get_MPI_rank();
    static int get_MPI_size();
    static int get_data(int key, int index_of_element);  // получить элемент по индексу с любого процесса
    static void set_data(int key, int index_of_element, int value);  // сохранить значение элемента по индексу с любого процесса
    static int create_object(int number_of_elements, int quantum_size = DEFAULT_QUANTUM_SIZE);  // создать новый memory_line и занести его в memory
    static int get_quantum_index(int key, int index);  // получить номер кванта по индексу
    static int get_quantum_size(int key);  // получить размер кванта
    static void set_lock(int key, int quantum_index);  // заблокировать квант
    static void unset_lock(int key, int quantum_index);  // разблокировать квант
    static void change_mode(int key, int quantum_index_l, int quantum_index_r, int mode);  // сменить режим работы с памятью
    static void read(int key, const std::string& path, int number_of_elements);  // прочитать из файла number_of_elements элементов
    static void read(int key, const std::string& path, int number_of_elements, int offset, int num_of_elem_proc); // прочитать из файла со смещением от начала, равным offset,number_of_elements элементов
    static void print(int key, const std::string& path);
    static void finalize();  // функция, завершающая выполнение программы, останавливает вспомогательные потоки
private:
    static void print_quantum(int key, int quantum_index);
    static int get_owner(int key, int quantum_index, int requesting_process);  // получить номер кванта, хранящего квант в текущий момент времени
    friend void worker_helper_thread();  // функция, выполняемая вспомогательными потоками процессов-рабочих
    friend void master_helper_thread();  // функция, выполняемая вспомогательным потоком процесса-мастера
};

#endif  // __MEMORY_MANAGER_H__
