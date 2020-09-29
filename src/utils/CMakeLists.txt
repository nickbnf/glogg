add_library(utils STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/persistentinfo.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/configuration.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/downloader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/decompressor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/klogg_version.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/readablesize.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/atomicflag.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/configuration.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/log.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/uuid.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/perfcounter.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/persistable.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/persistentinfo.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/openfilehelper.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/downloader.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/decompressor.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/klogg_version.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/readablesize.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/synchronization.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/styles.h)

set_target_properties(utils PROPERTIES AUTOMOC ON)

target_include_directories(utils PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(utils PUBLIC
        project_options
        project_warnings
        Qt5::Gui
        Qt5::Widgets
        Qt5::Network
        Qt5::Concurrent
        absl::optional
        absl::synchronization
        klogg_karchive
        whereami
        plog)

add_dependencies(utils generate_version)

