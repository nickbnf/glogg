add_library(roaring STATIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/roaring.c
    ${CMAKE_CURRENT_SOURCE_DIR}/include/roaring.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/roaring.hh)

target_include_directories(roaring SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)