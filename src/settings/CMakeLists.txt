set(settings_src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/persistentinfo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/configuration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/configuration.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/persistable.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/persistentinfo.h)

add_library(settings STATIC ${settings_src})

target_include_directories(settings PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(settings PUBLIC Qt5::Gui nonstd utils)

add_library(settings_portable STATIC ${settings_src})
target_include_directories(settings_portable PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(settings_portable PUBLIC Qt5::Gui nonstd utils)
target_compile_definitions(settings_portable PUBLIC -DKLOGG_PORTABLE)


