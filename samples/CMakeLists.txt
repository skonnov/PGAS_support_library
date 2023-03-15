set(samples
    dijkstra.cpp
    locks_simple_sample.cpp
    matrixmult_queue.cpp
    matrixmult_single.cpp
    matrixmult.cpp
    matrixvector.cpp
    parallel_reduce_sum_of_vector.cpp
    sample_pqueue.cpp
    single_sum_of_vector.cpp
    statistics.cpp
    sum_of_vector_struct.cpp
    sum_of_vector.cpp
)

foreach(sample ${samples})
    get_filename_component(samples_executable ${sample} NAME_WE)
    add_executable(${samples_executable} ${sample})
    
    target_link_libraries(${samples_executable}
    PRIVATE
        PGAS_SL
    )
endforeach()
