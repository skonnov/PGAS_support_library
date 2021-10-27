#ifndef __COMMON_H__
#define __COMMON_H__

#define DEFAULT_QUANTUM_SIZE 500

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
    GET_DATA_RW,
    GET_DATA_R,
    SET_INFO,
    GET_INFO,
    LOCK,
    UNLOCK,
    CHANGE_MODE,
    PRINT
};

enum error_codes {
    ERR_UNKNOWN       = -1,
    ERR_OUT_OF_BOUNDS = -2,
    ERR_NULLPTR       = -3,
    ERR_WRONG_RANK    = -4
};

#define CHECK(expression, error_code)                                                                                    \
    if(!(expression)) {                                                                                                  \
        int rank;                                                                                                        \
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);                                                                            \
        std::cout<<"Check error on process w/ MPI_rank #"<<rank<<", file: "<<__FILE__<<", line: "<<__LINE__<<std::endl;  \
        MPI_Abort(MPI_COMM_WORLD, error_code);                                                                           \
    }

#endif  // __COMMON_H__