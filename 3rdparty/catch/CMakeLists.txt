#version 2.12.1
add_library(catch INTERFACE)
target_include_directories(catch SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
