#ifndef __TRACE_H__
#define __TRACE_H__

#include <iostream>
#include <fstream>
#include <string>
#include "common.h"

#ifndef ENABLE_STATISTICS_COLLECTION
    #define ENABLE_STATISTICS_COLLECTION false
#endif

#if (ENABLE_STATISTICS_COLLECTION)
    #ifndef ENABLE_STATISTICS_EVERY_CACHE_MISSES
        #define ENABLE_STATISTICS_EVERY_CACHE_MISSES false
    #endif

    #ifndef ENABLE_STATISTICS_CACHE_MISSES_CNT
        #define ENABLE_STATISTICS_CACHE_MISSES_CNT false
    #endif

    #ifndef ENABLE_STATISTICS_QUANTUMS_SCHEDULE
        #define ENABLE_STATISTICS_QUANTUMS_SCHEDULE false
    #endif

    #ifndef ENABLE_STATISTICS_QUANTUMS_CNT_WORKERS
        #define ENABLE_STATISTICS_QUANTUMS_CNT_WORKERS false
    #endif

#endif

#ifndef STATISTICS_OUTPUT_DIRECTORY
    #define STATISTICS_OUTPUT_DIRECTORY std::string("trace_output/")
#endif

enum trace_mode {
    SINGLE_PROCESS_TRACE,
    MULTI_PROCESSES_TRACE
};

class trace_common {
    std::ofstream file_stream;
public:
    trace_common() {};
    StatusCode open_file(std::string filename) {
        file_stream.open(STATISTICS_OUTPUT_DIRECTORY + filename);
        if (file_stream.is_open() == false) {
            return STATUS_ERR_FILE_OPEN;
        }
        return STATUS_OK;
    }

    friend std::ofstream& operator<<(std::ofstream &out, std::string& str);

    ~trace_common() {
        if (file_stream.is_open()) {
            file_stream.close();
        }
    }
};

std::ofstream& operator<<(std::ofstream &out, std::string& str) {
    out << str;
    return out;
}

#endif  // __TRACE_H__
