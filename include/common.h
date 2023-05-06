#ifndef __COMMON_H__
#define __COMMON_H__

#include <string>
#include <fstream>
#include <iostream>

#define DEFAULT_QUANTUM_SIZE 500
#define DEFAULT_CACHE_SIZE 500

#ifndef ENABLE_STATISTICS_COLLECTION
    #define ENABLE_STATISTICS_COLLECTION true
#endif

#if (ENABLE_STATISTICS_COLLECTION)
    #ifndef ENABLE_STATISTICS_EVERY_CACHE_MISSES
        #define ENABLE_STATISTICS_EVERY_CACHE_MISSES true
    #endif

    #ifndef ENABLE_STATISTICS_CACHE_MISSES_CNT
        #define ENABLE_STATISTICS_CACHE_MISSES_CNT false
    #endif

    #ifndef ENABLE_STATISTICS_QUANTUMS_SCHEDULE
        #define ENABLE_STATISTICS_QUANTUMS_SCHEDULE true
    #endif

    #ifndef ENABLE_STATISTICS_QUANTUMS_CNT_WORKERS
        #define ENABLE_STATISTICS_QUANTUMS_CNT_WORKERS false
    #endif

#endif

enum mods {  // используется для изменения режима работы с памятью
    READ_ONLY,
    READ_WRITE
};

enum tags {  // используется для корректного распределения пересылок данных через MPI
    GET_DATA_FROM_HELPER             = 100,
    SEND_DATA_TO_HELPER              = 101,
    SEND_DATA_TO_MASTER_HELPER       = 102,
    GET_DATA_FROM_MASTER_HELPER_LOCK = 103,
    GET_INFO_FROM_MASTER_HELPER      = 104,
    GET_PERMISSION_FOR_CHANGE_MODE   = 105,
    GET_PERMISSION_TO_CONTINUE       = 106,
    REDUCE_TAG                       = 107,
    REDUCE_ALL_TAG1                  = 108,
    REDUCE_ALL_TAG2                  = 109,
    NOTIFY                           = 110,
    FINALIZE_WORKER                  = 111,
    FINALIZE_MASTER                  = 112
};

enum operations {  // используется вспомогательными потоками для определения типа запрашиваемой операции
    GET_DATA_RW = 0,
    GET_DATA_R  = 1,
    SET_INFO    = 2,
    GET_INFO    = 3,
    LOCK        = 4,
    UNLOCK      = 5,
    CHANGE_MODE = 6,
    PRINT       = 7,
    DELETE      = 8
};

enum StatusCode {
    STATUS_OK                          =  0,
    STATUS_ERR_UNKNOWN                 = -1,
    STATUS_ERR_OUT_OF_BOUNDS           = -2,
    STATUS_ERR_NULLPTR                 = -3,
    STATUS_ERR_WRONG_RANK              = -4,
    STATUS_ERR_ILLEGAL_WRITE           = -5,
    STATUS_ERR_READ_UNINITIALIZED_DATA = -6,
    STATUS_ERR_FILE_OPEN               = -7
};

static std::string get_error_code(int error_code) {
    std::string out;
    switch(error_code) {
    case STATUS_ERR_UNKNOWN:                 out = "STATUS_ERR_UNKNOWN"; break;
    case STATUS_ERR_OUT_OF_BOUNDS:           out = "STATUS_ERR_OUT_OF_BOUNDS"; break;
    case STATUS_ERR_NULLPTR:                 out = "STATUS_ERR_NULLPTR"; break;
    case STATUS_ERR_WRONG_RANK:              out = "STATUS_ERR_WRONG_RANK"; break;
    case STATUS_ERR_ILLEGAL_WRITE:           out = "STATUS_ERR_ILLEGAL_WRITE"; break;
    case STATUS_ERR_READ_UNINITIALIZED_DATA: out = "STATUS_ERR_READ_UNINITIALIZED_DATA"; break;
    case STATUS_ERR_FILE_OPEN:               out = "STATUS_ERR_FILE_OPEN"; break;
    default:;
    }
    out += "(" + std::to_string(error_code) + ")";
    return out;
}
#define CHECK(expression, error_code)                                                                                \
    if (!(expression)) {                                                                                             \
        int rank;                                                                                                    \
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);                                                                        \
        std::cout << "Check failed on process w/ MPI_rank #" << rank << ", file: "                                   \
             << __FILE__ << ", line: " << __LINE__ <<  " with error "  << get_error_code(error_code) << std::endl;   \
        MPI_Abort(MPI_COMM_WORLD, error_code);                                                                       \
    }

#define ABORT(error_code)                                                                                            \
    if (true) {                                                                                                      \
        int rank;                                                                                                    \
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);                                                                        \
        std::cout << "Abort was called on process w/ MPI_rank #" << rank << ", file: "                               \
             << __FILE__ << ", line: " << __LINE__ <<  " with error "  << get_error_code(error_code) << std::endl;   \
        MPI_Abort(MPI_COMM_WORLD, error_code);                                                                       \
    }

// #define PRINT_TO_FILE(path, filename, info)                                                    \
//     if (true) {                                                                                \
//         int rank;                                                                              \
//         MPI_Comm_rank(MPI_COMM_WORLD, &rank);                                                  \
//         file.open(path + filename + "_"  + std::to_string(rank) + ".txt", std::ios_base::app); \
//         file << info;                                                                          \
//         file.close();                                                                          \
//     }


#endif  // __COMMON_H__
