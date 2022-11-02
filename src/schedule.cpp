#include <schedule.h>
#include <iostream>
#include <fstream>
#include <vector>
std::vector<int> parse_string_line(const std::string& line) {
    size_t pos = 0;
    std::string token;
    size_t cur = 0;
    std::vector<int> values;
    while ((pos = line.find(" ")) != std::string::npos) {
        token = line.substr(cur, pos);
        if (token[0] == 'C')  // CHANGE_MODE ?
        {
            values.push_back(-1);
        }
        values.push_back(std::stoi(token));
        cur = pos;
    }
    return values;
}

void schedule::read_from_file(std::string path) {
    std::ifstream in("in.txt");
    std::string line;
    while(std::getline(std::cin, line))
    {
        std::vector<int> parsed_line = parse_string_line(line);
        int key = parsed_line[1];
        if (parsed_line[0] == -1) {  // CHANGE_MODE
            int l = parsed_line[2], r = parsed_line[3];
        } else {
            int quantum_number = parsed_line[2], process_number = parsed_line[3];
        }
        // std::cout << line << "\n";
    }
}
