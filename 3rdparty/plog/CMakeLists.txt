#version 1.1.5
add_library(plog INTERFACE)
target_include_directories(plog SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
