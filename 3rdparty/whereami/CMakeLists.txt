add_library(whereami STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/whereami/whereami.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/whereami.c)

target_include_directories(whereami SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(whereami SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/whereami)