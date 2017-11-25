add_library(gtest STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/gtest-all.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/gmock-all.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/gmock_main.cc"
)
target_include_directories(gtest SYSTEM PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(gtest SYSTEM PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

if(UNIX)
    target_link_libraries(gtest PUBLIC pthread)
endif()

set_target_properties(gtest PROPERTIES FOLDER "external")
