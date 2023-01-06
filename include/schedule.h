#ifndef __SCHEDULE_H__
#define __SCHEDULE_H__

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include "common.h"

enum schedule_enum {
    SCHEDULE_CHANGE_MODE, SCHEDULE_GET_QUANTUM
};

struct schedule_line
{
    schedule_enum mode;
    union {
        int l;
        int quantum_number;
    };
    union {
        int r;
        int number_of_process;
    };
};

class schedule {
    std::unordered_map<int, std::vector<schedule_line>> schedule_structure;
    std::unordered_map<int, std::unordered_map<int, std::vector<int>>> quantum_cnts;
    std::unordered_map<int, std::unordered_map<int, std::vector<int>>> modes;
public:
    StatusCode read_from_file_schedule(std::string path);
    StatusCode read_from_file_quantums_access_cnt(std::string path);
    void optimize();
    std::unordered_map<int, std::vector<schedule_line>> get_schedule() const;
private:
    std::vector<int> parse_string_line_schedule(const std::string& line);
    std::vector<int> parse_string_line_quantums_access_cnt(const std::string& line);
};
#endif
