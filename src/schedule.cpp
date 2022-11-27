#include <iostream>
#include <fstream>
#include <vector>
#include "schedule.h"
#include "common.h"

std::vector<int> schedule::parse_string_line_schedule(const std::string& line) {
    size_t pos = 0;
    std::string token;
    size_t cur = 0;
    std::vector<int> values;
    while ((pos = line.find(" ", cur)) != std::string::npos) {
        token = line.substr(cur, pos - cur);
        if (token[0] == 'C') {  // CHANGE_MODE ?
            values.push_back(-1);
        } else {
            values.push_back(std::stoi(token));
        }
        cur = pos + 1;
    }
    token = line.substr(cur, pos - cur);
    values.push_back(std::stoi(token));
    return values;
}

StatusCode schedule::read_from_file_schedule(std::string path) {
    std::ifstream in(path);
    if (!in) {
        return STATUS_ERR_FILE_OPEN;
    }
    std::string line;
    while(std::getline(in, line)) {
        std::vector<int> parsed_line = parse_string_line_schedule(line);
        int key = parsed_line[0];
        schedule_structure[key]; // create vector on key, if it didn't exist yet;
        if (parsed_line[1] == -1) {  // CHANGE_MODE
            int l = parsed_line[2], r = parsed_line[3];
            schedule_structure[key].push_back({schedule_enum::SCHEDULE_CHANGE_MODE, l, r});
        } else {
            int quantum_number = parsed_line[1], process_number = parsed_line[2];
            schedule_structure[key].push_back({schedule_enum::SCHEDULE_GET_QUANTUM, quantum_number, process_number});
        }
    }

    return STATUS_OK;
}

std::vector<int> schedule::parse_string_line_quantums_access_cnt(const std::string& line) {
    size_t pos = 0;
    std::string token;
    size_t cur = 0;
    std::vector<int> values;
    while ((pos = line.find(" ", cur)) != std::string::npos) {
        token = line.substr(cur, pos - cur);
        values.push_back(std::stoi(token));
        cur = pos + 1;
    }
    token = line.substr(cur, pos - cur);
    values.push_back(std::stoi(token));
    return values;
}

StatusCode schedule::read_from_file_quantums_access_cnt(std::string path) {
    std::ifstream in(path);
    if (!in) {
        return STATUS_ERR_FILE_OPEN;
    }

    std::string line;
    std::getline(in, line); // skip first line
    while(std::getline(in, line)) {
        std::vector<int> parsed_line = parse_string_line_quantums_access_cnt(line);
        int key = parsed_line[0], quantum_index = parsed_line[1], cnt = parsed_line[2], mode = parsed_line[3];
        quantum_cnts[key]; // create structure on key, if it didn't exist yet;
        quantum_cnts[key][quantum_index]; // create vector on quantum_index, if it didn't exist yet;
        modes[key]; // create structure on key, if it didn't exist yet;
        modes[key][quantum_index]; // create vector on quantum_index, if it didn't exist yet;
        quantum_cnts[key][quantum_index].push_back(cnt);
        modes[key][quantum_index].push_back(cnt);
    }

    return STATUS_OK;
}

void schedule::optimize() {
    // to do some optimizations
}

std::unordered_map<int, std::vector<schedule_line>> schedule::get() const {
    return schedule_structure;
}
