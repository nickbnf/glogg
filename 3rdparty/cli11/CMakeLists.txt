#version 1.9.0
add_library(cli11 INTERFACE)
target_include_directories(cli11 SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(cli11 INTERFACE -DCLI11_HAS_FILESYSTEM=0)
