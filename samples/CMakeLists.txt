file(GLOB samples RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)
foreach(sample ${samples})
    get_filename_component(exe ${sample} NAME_WE)
    add_executable(${exe} ${sample})
    target_link_libraries(${exe} PGAS_SL)
    if(MPI_FOUND)
        target_link_libraries(${exe} ${MPI_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

        if(MPI_COMPILE_FLAGS)
        set_target_properties(${exe} PROPERTIES COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
        endif()

        if(MPI_LINK_FLAGS)
        set_target_properties(${exe} PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}")
        endif()
    else(MPI_FOUND)
        target_link_libraries(${exe} MPI_LIB ${CMAKE_THREAD_LIBS_INIT})
    endif(MPI_FOUND)
endforeach()
