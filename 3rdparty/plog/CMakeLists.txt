#version 1.1.3
add_library(plog INTERFACE)
target_include_directories(plog SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
