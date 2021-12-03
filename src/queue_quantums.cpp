#include "queue_quantums.h"



queue_quantums::queue_quantums(int num_quantums) {
    if (num_quantums != 0)
        v_queues.resize(num_quantums);
}

void queue_quantums::push(int quantum_number, int process) {
    if (quantum_number < 0 || quantum_number >= (int)v_queues.size())
        throw -1;
    v_queues[quantum_number].push(process);
}

int queue_quantums::pop(int quantum_number) {
    if (quantum_number < 0 || quantum_number >= (int)v_queues.size())
        throw -1;
    if (!is_contain(quantum_number))
        throw -2; // do list of errors!
    int process =  v_queues[quantum_number].front();
    v_queues[quantum_number].pop();
    return process;
}

bool queue_quantums::is_contain(int quantum_number) {
    if (quantum_number < 0 || quantum_number >= (int)v_queues.size())
        throw -1;
    return !v_queues[quantum_number].empty();
}

void queue_quantums::resize(int num_quantums) {
    v_queues.resize(num_quantums);
}