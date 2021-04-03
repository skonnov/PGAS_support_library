file(GLOB hdrs "../include/*.h")
file(GLOB srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp")

add_library(PGAS_SL ${hdrs} ${srcs})
