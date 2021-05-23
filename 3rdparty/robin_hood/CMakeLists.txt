add_library(robin_hood INTERFACE)
target_include_directories(robin_hood SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
