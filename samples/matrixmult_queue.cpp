#include <iostream>
#include "memory_manager.h"
#include "parallel_vector.h"

int main(int argc, char** argv) {
    memory_manager::init(argc, argv);
    memory_manager::finalize();
    return 0;
}