cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(parallel_arrays C CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_library(PGAS_SL SHARED)

find_package(Threads REQUIRED)

find_package(MPI)

if(MPI_FOUND)
    include_directories(${MPI_INCLUDE_PATH})
else(MPI_FOUND)
    message("MPI not found by find_package, try to find MPI by your paths")
    if ($ENV{MPI_PATH})
        set(PATH_TO_MPI $ENV{MPI_PATH})
    else()
        set(PATH_TO_MPI "C:/Program Files (x86)/Microsoft SDKs/MPI")
    endif()

    set(MPI_INCLUDE_PATH ${PATH_TO_MPI}"/Include")
    set(MPI_LIB_PATH ${PATH_TO_MPI}"/Lib/x86/msmpi.lib")

    include_directories(${MPI_INCLUDE_PATH})
    add_library(MPI_LIBRARIES STATIC IMPORTED)
    set_property(TARGET MPI_LIBRARIES PROPERTY IMPORTED_LOCATION ${MPI_LIB_PATH})
endif()

target_include_directories(PGAS_SL
PUBLIC
    include
)

target_sources(PGAS_SL
PRIVATE
    include/cache_list.h
    include/common.h
    include/detail.h
    include/memory_allocator.h
    include/memory_cache.h
    include/memory_manager.h
    include/parallel_priority_queue.h
    include/parallel_reduce_all.h
    include/parallel_reduce.h
    include/parallel_vector.h
    include/queue_quantums.h
    include/schedule.h

    src/cache_list.cpp
    src/memory_allocator.cpp
    src/memory_cache.cpp
    src/memory_manager.cpp
    src/queue_quantums.cpp
    src/schedule.cpp
)

if(MPI_COMPILE_FLAGS)
    set_target_properties(PGAS_SL PROPERTIES COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
endif()

if(MPI_LINK_FLAGS)
    set_target_properties(PGAS_SL PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}")
endif()

target_link_libraries(PGAS_SL
PUBLIC
    ${CMAKE_THREAD_LIBS_INIT}
    ${MPI_LIBRARIES}
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})

add_subdirectory(samples)
