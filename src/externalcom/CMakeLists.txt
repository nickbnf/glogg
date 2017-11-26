if (${PREFER_DBUS_IPC})
    find_package(DBus)
    find_package(Qt5DBus)
endif()

add_library(externalcom STATIC 
    "${CMAKE_CURRENT_SOURCE_DIR}/include/externalcom.h")

if (${DBUS_FOUND} AND ${Qt5DBus_FOUND})
    message("Support for D-BUS will be included")
    target_sources(externalcom PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include/dbuscontrol.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/dbusexternalcom.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/dbusexternalcom.cpp")
    target_link_libraries(externalcom PUBLIC Qt5::DBus)
    target_compile_definitions(externalcom PUBLIC -DGLOGG_SUPPORTS_DBUS)
else()
    find_package(Qt5Network REQUIRED)

    message("Support for cross-platform IPC will be included")
    target_sources(externalcom PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include/socketexternalcom.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/socketexternalcom.cpp")
    target_link_libraries(externalcom PUBLIC Qt5::Network)
    target_compile_definitions(externalcom PUBLIC -DGLOGG_SUPPORTS_SOCKETIPC)

endif()

target_include_directories(externalcom PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(externalcom PUBLIC utils)
