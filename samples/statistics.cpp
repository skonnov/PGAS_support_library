#include <iostream>
#include "schedule.h"
#include "common.h"
#include "memory_manager.h"

int main(int argc, char** argv) {
    memory_manager::init(argc, argv);
    schedule sch;
    StatusCode sts = StatusCode::STATUS_OK;
    if (memory_manager::get_MPI_rank() > 0) {
        sts = sch.read_from_file_quantums_access_cnt("/mnt/d/Works/PGAS_support_library/build_linux/quantums_ranks_cnt_process_" +
            std::to_string(memory_manager::get_MPI_rank()) + "_1.txt");
    } else {
        sts = sch.read_from_file_schedule("/mnt/d/Works/PGAS_support_library/build_linux/quantums_schedule_raw_1.txt");
    }
    if (sts != STATUS_OK) {
        std::cout << "rank: " << memory_manager::get_MPI_rank() << ", status code: " << get_error_code(sts) << " :(" << std::endl;
    }
    memory_manager::finalize();
    return 0;
}
