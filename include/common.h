#ifndef __COMMON_H__
#define __COMMON_H__

#define QUANTUM_SIZE 500

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
    REDUCE_TAG                       = 106
};

enum operations {  // используется вспомогательными потоками для определения типа запрашиваемой операции
    GET_DATA_RW,
    GET_DATA_R,
    SET_INFO,
    GET_INFO,
    LOCK,
    UNLOCK,
    CHANGE_MODE
};

#endif  // __COMMON_H__