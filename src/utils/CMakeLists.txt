add_library(utils STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/persistentinfo.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/configuration.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/atomicflag.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/configuration.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/log.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/perfcounter.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/persistable.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/persistentinfo.h
)

target_include_directories(utils PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(utils PUBLIC Qt5::Gui nonstd plog)
