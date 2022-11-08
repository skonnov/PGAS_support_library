#include "queue_quantums.h"



queue_quantums::queue_quantums(int num_quantums) {
    if (num_quantums != 0)
        v_queues.resize(num_quantums);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
}

void queue_quantums::push(int quantum_number, int process) {
    CHECK(quantum_number >= 0 && quantum_number < (int)v_queues.size(), STATUS_ERR_OUT_OF_BOUNDS);
    v_queues[quantum_number].push(process);
}

int queue_quantums::pop(int quantum_number) {
    CHECK(quantum_number >= 0 && quantum_number < (int)v_queues.size(), STATUS_ERR_OUT_OF_BOUNDS);
    CHECK(is_contain(quantum_number), STATUS_ERR_UNKNOWN); // make another new error?
    int process =  v_queues[quantum_number].front();
    v_queues[quantum_number].pop();
    return process;
}

bool queue_quantums::is_contain(int quantum_number) {
    CHECK(quantum_number >= 0 && quantum_number < (int)v_queues.size(), STATUS_ERR_OUT_OF_BOUNDS);
    return !v_queues[quantum_number].empty();
}

void queue_quantums::resize(int num_quantums) {
    v_queues.resize(num_quantums);
}
