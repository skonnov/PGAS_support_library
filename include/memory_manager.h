#ifndef __MEMORY_MANAGER_H__
#define __MEMORY_MANAGER_H__

#include <map>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <iostream>
#include "common.h"
#include "queue_quantums.h"

void worker_helper_thread();
void master_helper_thread();

struct memory_line_common {
    std::vector<int> num_change_mode;  // для перехода между режимами
    int logical_size;  // общее число элементов в векторе на всех процессах
    virtual ~memory_line_common() {}
};

struct memory_line_worker
    : public memory_line_common {
    std::vector<int*>quantums;  // вектор указателей на кванты
    std::vector<std::mutex*> mutexes;  // мьютексы на каждый квант, нужны, чтобы предотвратить одновременный доступ
                                       // к кванту с разных потоков в режиме READ_WRITE
};

struct memory_line_master
    : public memory_line_common {
    std::vector<std::pair<bool, int>>quantum_owner; // для read_write mode, массив пар: bool - готов ли квант для передачи;
                                                                                    // int - на каком процессе расположен квант
    queue_quantums wait_locks;  // мапа очередей для процессов, ожидающих разблокировки кванта, заблокированных через set_lock
    queue_quantums wait_quantums;  // мапа очередей для процессов, ожидающих разблокировки кванта, заблокированных процессом-мастером
    std::vector<int> quantums_for_lock;  // вектор для определения номеров процессов, блокирующих кванты
    std::vector<std::vector<int>> owners; // для read_only mode, номера процессов, хранящих у себя квант

};

class memory_manager {
    static std::vector<memory_line_common*> memory;  // структура-хранилище памяти и вспомогательной информации
    static std::thread helper_thr;  // вспомогательный поток
    static int rank, size;  // ранг процесса в MPI и число процессов
    static int worker_rank, worker_size;  // worker_rank = rank-1, worker_size = size-1
    static bool is_read_only_mode;  // активен ли режим READ_ONLY
    static int num_of_change_mode_procs;  // число процессов, обратившихся к мастеру для изменения режима работы
    static int num_of_change_mode;  // общее число изменений режима работы
    static std::vector<long long> times;  // вектор, сохраняющий информацию, когда в последний раз было обращение к какому-либо процессу
    static long long time;  // вспомогательный счётчик для вектора times
public:
    static void memory_manager_init(int argc, char** argv);  // функция, вызываемая в начале выполнения программы, инициирует вспомогательные потоки
    static int get_MPI_rank();
    static int get_MPI_size();
    static int get_data(int key, int index_of_element);  // получить элемент по индексу с любого процесса
    static void set_data(int key, int index_of_element, int value);  // сохранить значение элемента по индексу с любого процесса
    static int create_object(int number_of_elements);  // создать новый memory_line и занести его в memory
    static int get_quantum_index(int index);  // получить номер кванта по индексу
    static void set_lock(int key, int quantum_index);  // заблокировать квант
    static void unset_lock(int key, int quantum_index);  // разблокировать квант
    static void change_mode(int mode);  // сменить режим работы с памятью
    static void finalize();  // функция, завершающая выполнение программы, останавливает вспомогательные потоки
private:
    static int get_owner(int key, int quantum_index, int requesting_process);
    static bool is_mode_changed(int key, int quantum_index);
    friend void worker_helper_thread();  // функция, выполняемая вспомогательными потоками процессов-рабочих
    friend void master_helper_thread();  // функция, выполняемая вспомогательным потоком процесса-мастера
};

#endif  // __MEMORY_MANAGER_H__
