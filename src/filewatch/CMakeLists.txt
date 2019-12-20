add_library(filewatch STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/filewatcher.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/filewatcher.cpp
)

set_target_properties(filewatch PROPERTIES AUTOMOC ON)

target_include_directories(filewatch PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(filewatch PUBLIC efsw utils)
