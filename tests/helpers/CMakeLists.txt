add_executable(file_write_helper
        file_write_helper.cpp
        file_write_helper.h
)

target_link_libraries(file_write_helper utils)

add_library(test_utils INTERFACE)
target_include_directories(catch SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})