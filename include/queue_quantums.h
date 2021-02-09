#ifndef __QUEUE_QUANTUMS_H__
#define __QUEUE_QUANTUMS_H__

#include <vector>
#include <queue>

class queue_quantums
{
    std::vector<std::queue<int>> v_queues;  // вектор очередей, хранящих номера процессов, которые ждут освобождения квантов
public:
    queue_quantums(int num_quantums = 0);
    void push(int quantum_number, int process);
    int  pop(int quantum_number);
    bool is_contain(int quantum_number);
    void resize(int num_quantums);
};

#endif  // __QUEUE_QUANTUMS_H__