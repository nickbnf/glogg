#version 1.1.1
add_executable(maddy main.cpp)
target_include_directories(maddy SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
