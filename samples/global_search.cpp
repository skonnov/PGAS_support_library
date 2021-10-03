#include <iostream>
#include <memory_manager.h>
#include <parallel_priority_queue.h>

double eps = 0.01;

int global_search(std::vector<std::function<int(int)>> g, std::function<int(int)> f) {
    return 0;
}

int main(int argc, char** argv) {
    memory_manager::init(argc, argv);
    memory_manager::finalize();
    double x1 = 0, x2 = 5; // temp;
    return 0;
}