#include <schedule.h>
#include <iostream>
#include <fstream>
#include <vector>

std::vector<int> parse_string_line(const std::string& line) {
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

StatusCode schedule::read_from_file(std::string path) {
    std::ifstream in(path);
    if (!in) {
        return STATUS_ERR_FILE_OPEN;
    }
    std::string line;
    while(std::getline(in, line))
    {
        std::vector<int> parsed_line = parse_string_line(line);
        int key = parsed_line[1];
        if (schedule_structure.find(key) == schedule_structure.end()) {
            schedule_structure[key] = new std::vector<schedule_line>();
        }
        if (parsed_line[0] == -1) {  // CHANGE_MODE
            int l = parsed_line[2], r = parsed_line[3];
            schedule_structure[key]->push_back({schedule_enum::SCHEDULE_CHANGE_MODE, l, r});
        } else {
            int quantum_number = parsed_line[2], process_number = parsed_line[3];
            schedule_structure[key]->push_back({schedule_enum::SCHEDULE_GET_QUANTUM, quantum_number, process_number});
        }
    }

    return STATUS_OK;
}

void schedule::optimize() {
    // to do some optimizations
}

std::unordered_map<int, std::vector<schedule_line>*> schedule::get() const {
    return schedule_structure;
}

schedule::~schedule() {
    for (auto i: schedule_structure) {
        delete i.second;
    }
}
