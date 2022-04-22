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
#include <memory>
#include <mpi.h>
#include "common.h"
#include "detail.h"
#include "memory_allocator.h"
#include "queue_quantums.h"

void worker_helper_thread();
void master_helper_thread();

struct quantum_common {
    int mode = READ_WRITE;
    bool is_mode_changed = false;
    int num_of_changed_mode_procs = 0;
    virtual ~quantum_common() {}
};

struct quantum_worker
    : public quantum_common {
    void* quantum = nullptr; // указатель на квант
    std::unique_ptr<std::mutex> mutex;  // мьютекс нужен, чтобы предотвратить одновременный доступ
                                    // к кванту с разных потоков в режиме READ_WRITE
    quantum_worker(): mutex(new std::mutex()) {}
};

struct quantum_master
    : public quantum_common {
    bool quantum_ready = false;  // готов ли квант для передачи
    int quantum_lock_number = -1;  // для хранения номера процесса, заблокировавшего квант через set_lock
    std::deque<int> owners;  // для read_only mode, номера процессов, хранящих у себя квант
    std::vector<int> requests; // хранит число текущих запросов по данному кванту для каждого процесса

    quantum_master(int number_of_procs): quantum_common() {
        requests.resize(number_of_procs, 0);
    }
};

struct memory_line_common {
    int logical_size;  // общее число элементов в векторе на всех процессах
    int quantum_size;
    virtual ~memory_line_common() {}
};

struct memory_line_worker
    : public memory_line_common {
    std::vector<quantum_worker> quantums;
    memory_allocator allocator;
    MPI_Datatype type;
    int size_of;
};

struct memory_line_master
    : public memory_line_common {
    std::vector<quantum_master> quantums;
    queue_quantums wait_locks;  // мапа очередей для процессов, ожидающих разблокировки кванта, заблокированных через set_lock
    queue_quantums wait_quantums;  // мапа очередей для процессов, ожидающих разблокировки кванта, заблокированных процессом-мастером

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
    static void init(int argc, char** argv, std::string error_helper = "");  // функция, вызываемая в начале выполнения программы, инициирует вспомогательные потоки
    static int get_MPI_rank();
    static int get_MPI_size();
    template <class T> static T get_data(int key, int index_of_element);  // получить элемент по индексу с любого процесса
    template <class T> static void set_data(int key, int index_of_element, T value);  // сохранить значение элемента по индексу с любого процесса
    template <class T> static int create_object(int number_of_elements, int quantum_size, int count, const int* blocklens, const MPI_Aint* indices, const MPI_Datatype* types);
    template <class T> static int create_object(int number_of_elements, int quantum_size = DEFAULT_QUANTUM_SIZE);  // создать новый memory_line и занести его в memory
    static int get_quantum_index(int key, int index);  // получить номер кванта по индексу
    static int get_quantum_size(int key);  // получить размер кванта
    static void set_lock(int key, int quantum_index);  // заблокировать квант
    static void unset_lock(int key, int quantum_index);  // разблокировать квант
    static void change_mode(int key, int quantum_index_l, int quantum_index_r, mods mode);  // сменить режим работы с памятью
    template <class T> static void read(int key, const std::string& path, int number_of_elements);  // прочитать из файла number_of_elements элементов
    template <class T> static void read(int key, const std::string& path, int number_of_elements, int offset, int num_of_elem_proc); // прочитать из файла со смещением от начала, равным offset,number_of_elements элементов
    static void print(int key, const std::string& path);
    static MPI_Datatype get_MPI_datatype(int key);
    static void finalize();  // функция, завершающая выполнение программы, останавливает вспомогательные потоки
    static void wait_all();
    static void wait_all_workers();
    static int wait();  // перевести процесс в состояние ожидания до тех пор, пока для него не будет вызвана функция notify. Возвращает номер процесса, вызвавшего notify
    static void wait(int from_rank); // перевести процесс в состояние ожидания до тех пор, пока процесс from_rank не вызовет функцию notify
    static void notify(int to_rank);  // возобновить работу процесса rank
private:
    static void print_quantum(int key, int quantum_index);
    static int get_owner(int key, int quantum_index, int requesting_process);  // получить номер кванта, хранящего квант в текущий момент времени
    friend void worker_helper_thread();  // функция, выполняемая вспомогательными потоками процессов-рабочих
    friend void master_helper_thread();  // функция, выполняемая вспомогательным потоком процесса-мастера
};

template <class T>
int memory_manager::create_object(int number_of_elements, int quantum_size) {
    memory_line_common* line;
    int num_of_quantums = (number_of_elements + quantum_size - 1) / quantum_size;
    if (rank == 0) {
        line = new memory_line_master;
        auto line_master = dynamic_cast<memory_line_master*>(line);
        line_master->quantums.resize(num_of_quantums, quantum_master(size));
        line_master->wait_locks.resize(num_of_quantums);
        line_master->wait_quantums.resize(num_of_quantums);
    } else {
        line = new memory_line_worker;
        auto line_worker = dynamic_cast<memory_line_worker*>(line);
        line_worker->quantums.resize(num_of_quantums);
        line_worker->allocator.set_quantum_size(quantum_size, sizeof(T));
        line_worker->type = get_mpi_type<T>();
        line_worker->size_of = sizeof(T);
    }
    line->quantum_size = quantum_size;
    line->logical_size = number_of_elements;
    memory.emplace_back(line);
    MPI_Barrier(MPI_COMM_WORLD);
    return int(memory.size()) - 1;
}

template <class T>
int memory_manager::create_object(int number_of_elements, int quantum_size, int count, const int* blocklens, const MPI_Aint* indices, const MPI_Datatype* types) {
    int key = memory_manager::create_object<T>(number_of_elements, quantum_size);
    if (rank) {
        dynamic_cast<memory_line_worker*>(memory[key])->type = create_mpi_type<T>(count, blocklens, indices, types);
    }
    return key;
}

template <class T>
T memory_manager::get_data(int key, int index_of_element) {
    CHECK(key >= 0 && key < (int)memory_manager::memory.size(), ERR_OUT_OF_BOUNDS);
    auto* memory = dynamic_cast<memory_line_worker*>(memory_manager::memory[key]);
    int quantum_index = get_quantum_index(key, index_of_element);
    auto& quantum = memory->quantums[quantum_index].quantum;
    CHECK(index_of_element >= 0 && index_of_element < (int)memory->logical_size, ERR_OUT_OF_BOUNDS);
    CHECK(quantum_index >= 0 && quantum_index < (int)memory->quantums.size(), ERR_OUT_OF_BOUNDS);
    if (memory->quantums[quantum_index].mode == READ_WRITE)   // если read_write mode, то используем мьютекс на данный квант
        memory->quantums[quantum_index].mutex->lock();
    if (!memory->quantums[quantum_index].is_mode_changed) {  // не было изменения режима? (данные актуальны?)
        if (quantum != nullptr) {  // на данном процессе есть квант?
            T elem = (reinterpret_cast<T*>(quantum))[index_of_element % memory->quantum_size];
            if (memory->quantums[quantum_index].mode == READ_WRITE)
                memory->quantums[quantum_index].mutex->unlock();
            return elem;  // элемент возвращается без обращения к мастеру
        }
    }
    if (memory->quantums[quantum_index].mode == READ_WRITE)
        memory->quantums[quantum_index].mutex->unlock();
    int request[4] = {GET_INFO, key, quantum_index, -1};  // обращение к мастеру с целью получить квант
    MPI_Send(request, 4, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);
    int to_rank = -2;
    MPI_Status status;
    MPI_Recv(&to_rank, 1, MPI_INT, 0, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD, &status);  // получение ответа от мастера
    memory->quantums[quantum_index].is_mode_changed = false; // ???
    if (memory->quantums[quantum_index].mode == READ_ONLY && to_rank == rank) {  // если read_only_mode и данные уже у процесса,
                                                 // ответ мастеру о том, что данные готовы, отправлять не нужно
        CHECK(quantum != nullptr, ERR_NULLPTR);
        return (reinterpret_cast<T*>(quantum))[index_of_element % memory->quantum_size];
    }
    if (to_rank != rank) {  // если данные не у текущего процесса, инициируется передача данных от указанного мастером процесса
        if (quantum == nullptr) {
            quantum = memory->allocator.alloc();
        }
        CHECK(to_rank > 0 && to_rank < size, ERR_WRONG_RANK);
        CHECK(quantum != nullptr, ERR_NULLPTR);
        MPI_Recv(quantum, memory->quantum_size, memory->type, to_rank, GET_DATA_FROM_HELPER, MPI_COMM_WORLD, &status);
    }
    request[0] = SET_INFO;
    request[3] = (to_rank != rank) ? to_rank : -1;
    CHECK(quantum != nullptr, ERR_NULLPTR);
    T elem = (reinterpret_cast<T*>(quantum))[index_of_element % memory->quantum_size];
    MPI_Send(request, 4, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);  // уведомление мастера о том, что данные готовы для передачи другим процессам
    return elem;
}

template <class T>
void memory_manager::set_data(int key, int index_of_element, T value) {
    CHECK(key >= 0 && key < (int)memory_manager::memory.size(), ERR_OUT_OF_BOUNDS);
    auto* memory = dynamic_cast<memory_line_worker*>(memory_manager::memory[key]);
    int quantum_index = get_quantum_index(key, index_of_element);
    CHECK(memory->quantums[quantum_index].mode == READ_WRITE, ERR_ILLEGAL_WRITE);  // запись в READ_ONLY режиме запрещена
    CHECK(index_of_element >= 0 && index_of_element < (int)memory->logical_size, ERR_OUT_OF_BOUNDS);
    CHECK(quantum_index >= 0 && quantum_index < (int)memory->quantums.size(), ERR_OUT_OF_BOUNDS);
    auto& quantum = memory->quantums[quantum_index].quantum;
    memory->quantums[quantum_index].mutex->lock();
    if (!memory->quantums[quantum_index].is_mode_changed) {
        if (quantum != nullptr) {
            (reinterpret_cast<T*>(quantum))[index_of_element % memory->quantum_size] = value;
            memory->quantums[quantum_index].mutex->unlock();
            return;
        }
    }
    memory->quantums[quantum_index].mutex->unlock();
    int request[4] = {GET_INFO, key, quantum_index, -1};
    MPI_Send(request, 4, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);  // обращение к мастеру с целью получить квант
    int to_rank = -2;
    MPI_Status status;
    MPI_Recv(&to_rank, 1, MPI_INT, 0, GET_INFO_FROM_MASTER_HELPER, MPI_COMM_WORLD, &status);  // получение ответа от мастера
    memory->quantums[quantum_index].is_mode_changed = false;
    if (quantum == nullptr) {
        quantum = memory->allocator.alloc();
    }
    if (to_rank != rank) {  // если данные не у текущего процесса, инициируется передача данных от указанного мастером процесса
        CHECK(to_rank > 0 && to_rank < size, ERR_WRONG_RANK);
        MPI_Recv(quantum, memory->quantum_size, memory->type, to_rank, GET_DATA_FROM_HELPER, MPI_COMM_WORLD, &status);
    }
    request[0] = SET_INFO;
    request[3] = (to_rank != rank) ? to_rank : -1;
    (reinterpret_cast<T*>(quantum))[index_of_element % memory->quantum_size] = value;
    MPI_Send(request, 4, MPI_INT, 0, SEND_DATA_TO_MASTER_HELPER, MPI_COMM_WORLD);  // уведомление мастера о том, что данные готовы для передачи другим процессам
}

template <class T>
void memory_manager::read(int key, const std::string& path, int number_of_elements) {
    auto* memory = memory_manager::memory[key];
    int num_of_quantums = (number_of_elements + memory->quantum_size - 1) / memory->quantum_size;
    int offset = 0;
    for (int w_rank = 0; w_rank < worker_size; ++w_rank) {
        int quantum_portion = num_of_quantums / worker_size + (w_rank < num_of_quantums % worker_size?1:0);
        if (quantum_portion == 0)
            break;
        if (worker_rank == w_rank) {
            std::ifstream fs(path, std::ios::in | std::ios::binary);
            fs.seekg(offset * sizeof(int));
            T data;
            for (int i = 0; i < std::min(quantum_portion * memory->quantum_size, number_of_elements); ++i) {
                int logical_index = offset + i;
                fs.read((char*)&data, sizeof(data));
                memory_manager::set_data<T>(key, logical_index, data);
            }
        }
        offset += quantum_portion * memory->quantum_size;
    }
    MPI_Barrier(workers_comm);  // ???
}

template <class T>
void memory_manager::read(int key, const std::string& path, int number_of_elements, int offset, int num_of_elem_proc) {
    std::ifstream fs(path, std::ios::in | std::ios::binary);
    fs.seekg(offset * sizeof(int));
    T data;
    for (int i = 0; i < std::min(num_of_elem_proc, number_of_elements); ++i) {
        int logical_index = offset + i;
        fs.read((char*)&data, sizeof(data));
        memory_manager::set_data<T>(key, logical_index, data);
    }
}


#endif  // __MEMORY_MANAGER_H__
