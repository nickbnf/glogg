add_library(versioncheck STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/versionchecker.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/versionchecker.cpp
)

target_include_directories(versioncheck PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(versioncheck PUBLIC Qt5::Network utils)
