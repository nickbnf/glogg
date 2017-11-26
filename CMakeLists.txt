cmake_minimum_required(VERSION 3.2)

project(klogg)

set(out_dir "${CMAKE_BINARY_DIR}/output")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${out_dir}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${out_dir}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${out_dir}")

set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(ucm)

find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Concurrent REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (UNIX)
    ucm_add_flags(CXX -Wall -Wextra -O2 -g)
    #ucm_add_flags(CXX -Wshadow -Wnon-virtual-dtor
    #                  -Wno-unused-parameter
    #                  -Wno-unused-variable)

    option(GPROF "Build for gprof" OFF)
    if (${GPROF})
        ucm_add_flags(-pg -g)
        ucm_add_linker_flags(-pg)
    endif()

    option(PREFER_DBUS_IPC "Prefer dbus IPC if available" ON)

endif()

if (APPLE)
    ucm_add_flags(CXX -stdlib=libc++)
    set(CMAKE_OSX_DEPLOYMENT_TARGET 10.7)
endif()

if (WIN32)
    add_definitions(-DNOMINMAX)
endif()

if (MSVC)
    ucm_add_flags(CXX /O2)
endif()

option(PERF "Build for performance test" OFF)
if(${PERF})
    add_definitions(-DGLOGG_PERF_MEASURE_FPS)
endif()

set(BUILD_VERSION "" CACHE STRING "build version")
set(BUILD_NUMBER "" CACHE STRING "build number")

include_directories(${CMAKE_BINARY_DIR}/generated)

add_custom_target (generate_version ALL
    COMMAND ${CMAKE_COMMAND} -DBUILD_VERSION=${BUILD_VERSION} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.cmake
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.cmake
)

add_custom_target(ci_config SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/.travis.yml
    ${CMAKE_CURRENT_SOURCE_DIR}/appveyor.yml
)

add_custom_target(installation SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/klogg.nsi
    ${CMAKE_CURRENT_SOURCE_DIR}/klogg.desktop
)

add_custom_target(docs SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/README.md
    ${CMAKE_CURRENT_SOURCE_DIR}/COPYING
)

add_subdirectory(3rdparty)
add_subdirectory(src)

option(BUILD_TESTS "Build tests" ON)
if (${BUILD_TESTS})
    enable_testing()

    add_test(
        NAME klogg_smoke
        COMMAND klogg -platform offscreen -v
    )

    add_subdirectory(tests)   
endif()

# Installation and packaging

install(DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/images/hicolor/16x16
    DESTINATION share/icons/hicolor/16x16/apps
)

install(DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/images/hicolor/32x32
    DESTINATION share/icons/hicolor/32x32/apps
)

install(DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/images/hicolor/scalable
    DESTINATION share/icons/hicolor/scalable/apps
)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/COPYING
    ${CMAKE_CURRENT_SOURCE_DIR}/README.md
    DESTINATION share/doc/klogg)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/klogg.desktop
    DESTINATION share/applications
)

include(InstallRequiredSystemLibraries)

SET(CPACK_OUTPUT_FILE_PREFIX "packages")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Klogg log viewer")
SET(CPACK_PACKAGE_VENDOR "Anton Filimonov")
SET(CPACK_PACKAGE_CONTACT "anton.filimonov@gmail.com")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
SET(CPACK_PACKAGE_VERSION "1.1.3-r${BUILD_NUMBER}")
SET(CPACK_STRIP_FILES "bin/klogg")

if(UNIX)
    if(APPLE)
        set(CPACK_GENERATOR DragNDrop ${CPACK_GENERATOR})
        set(CPACK_GENERATOR "DragNDrop")
        set(CPACK_DMG_FORMAT "UDBZ")
        set(CPACK_DMG_VOLUME_NAME "${PROJECT_NAME}")
        set(CPACK_SYSTEM_NAME "OSX")
        set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/src/app/images/glogg.icns")
    else()
        # linux
        IF(CPACK_GENERATOR STREQUAL "RPM")
            set(CPACK_RPM_PACKAGE_LICENSE "GPLv3")
            set(CPACK_RPM_PACKAGE_REQUIRES "qt5-qtbase")
            set(CPACK_RPM_PACKAGE_URL "http://github.com/variar/klogg")
        ELSE()
            set(CPACK_GENERATOR DEB)
            set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
            set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://github.com/variar/klogg")
            set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5dbus5, libqt5concurrent5, libqt5widgets5")
        ENDIF()
    endif()
    INCLUDE(CPack)
endif()




