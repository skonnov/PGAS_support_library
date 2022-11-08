#include <iostream>
#include "schedule.h"
#include "common.h"

int main() {
    schedule sch;
    StatusCode sts = sch.read_from_file("/mnt/d/Works/PGAS_support_library/build_linux/quantums_schedule_raw.txt");
    if (sts != STATUS_OK) {
        std::cout << get_error_code(sts) << " :(" << std::endl;
    }
    return 0;
}
