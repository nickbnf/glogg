INCLUDE (CheckSymbolExists)
INCLUDE (CheckCXXSourceCompiles)
INCLUDE (CheckCXXCompilerFlag)

cmake_minimum_required(VERSION 2.8.12)
project(glogg_tests)

# Qt

find_program(QT_QMAKE_EXECUTABLE NAMES qmake PATHS $ENV{QT_DIR}/bin)
find_package(Qt5Core 5.8 REQUIRED)
find_package(Qt5Widgets 5.8 REQUIRED)
find_package(Qt5Test 5.8 REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

check_cxx_compiler_flag(-Wall HAS_WALL)

if(HAS_WALL)
    set(PARANOID_FLAGS "${PARANOID_FLAGS} -Wall")
endif(HAS_WALL)

# Compiler
check_cxx_compiler_flag("-std=c++11" COMPILER_SUPPORTS_CXX11)
check_cxx_compiler_flag("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support.")
endif()

set(CMAKE_REQUIRED_FLAGS "-std=c++11")
check_cxx_source_compiles("
    #include <memory>
    int main(int, int)
    {
        auto test = std::make_unique<void>();
        return 0;
    }
" HAVE_MAKE_UNIQUE)

check_cxx_source_compiles("
    #include <memory>
    struct Base {
        virtual void func() {}
    };
    struct Derived : public Base {
        void func() override {}
    }
    int main(int, int)
    {
        return 0;
    }
" HAVE_OVERRIDE)

configure_file(../config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# Configuration
check_symbol_exists( symlink "unistd.h" HAVE_SYMLINK )

# find_library(WS2_LIBRARY NAMES ws2_32)

# Libs

set(LIBS gmock gtest)

# Setup testing
enable_testing()
include_directories(
    $ENV{GMOCK_HOME}/include
    $ENV{GMOCK_HOME}/gtest/include
    $ENV{BOOST_ROOT}/
    ../src/
)
link_directories($ENV{GMOCK_HOME}/mybuild $ENV{GMOCK_HOME}/mybuild/gtest)

# Sources
set(glogg_SOURCES
    ../src/session.cpp
    ../src/data/abstractlogdata.cpp
    ../src/data/logdata.cpp
    ../src/data/logfiltereddata.cpp
    ../src/data/logfiltereddataworkerthread.cpp
    ../src/data/logdataworkerthread.cpp
    ../src/data/compressedlinestorage.cpp
    ../src/mainwindow.cpp
    ../src/crawlerwidget.cpp
    ../src/abstractlogview.cpp
    ../src/logmainview.cpp
    ../src/filteredview.cpp
    ../src/optionsdialog.cpp
    ../src/persistentinfo.cpp
    ../src/configuration.cpp
    ../src/filtersdialog.cpp
    ../src/filterset.cpp
    ../src/savedsearches.cpp
    ../src/infoline.cpp
    ../src/menuactiontooltipbehavior.cpp
    ../src/selection.cpp
    ../src/quickfind.cpp
    ../src/quickfindpattern.cpp
    ../src/quickfindwidget.cpp
    ../src/sessioninfo.cpp
    ../src/recentfiles.cpp
    ../src/overview.cpp
    ../src/overviewwidget.cpp
    ../src/marks.cpp
    ../src/quickfindmux.cpp
    ../src/signalmux.cpp
    ../src/tabbedcrawlerwidget.cpp
    ../src/watchtowerlist.cpp
    ../src/watchtower.cpp
    ../src/viewtools.cpp
    ../src/encodingspeculator.cpp
    ../src/platformfilewatcher.cpp
    ../src/filewatcher.cpp
)

set(glogg_HEADERS
    ../src/mainwindow.h
    ../src/marks.h
    ../src/platformfilewatcher.h
    ../src/filewatcher.h
)

# Unit tests
set(glogg_UTESTS
    watchtowerTest.cpp
    linepositionarrayTest.cpp
    encodingspeculatorTest.cpp
)

# Integration tests
set(glogg_ITESTS
    logdataTest.cpp
    logfiltereddataTest.cpp
)

# Performance tests
set(glogg_PTESTS
    logdataPerfTest.cpp
    logfiltereddataPerfTest.cpp
)


# Options
if (WIN32)
    set(FileWatcherEngine_SOURCES
        ../src/winwatchtowerdriver.cpp
    )
elseif (LINUX)
    # set(FileWatcherEngine_SOURCES ../src/inotifywatchtowerdriver.cpp)
elseif (APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGLOGG_SUPPORTS_KQUEUE")
    set(FileWatcherEngine_SOURCES
         ../src/kqueuewatchtowerdriver.cpp
    )
endif (WIN32)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PARANOID_FLAGS} -fPIC -DGLOGG_VERSION=\\\"unit_tests\\\" -g -gdwarf-2")

# Add test cpp file
add_executable(glogg_tests
    ${glogg_SOURCES}
    ${FileWatcherEngine_SOURCES}
    ${glogg_UTESTS}
)

# Link test executable against gtest & gtest_main
target_link_libraries(glogg_tests ${LIBS} gtest_main pthread Qt5::Widgets)

add_executable(glogg_itests
    ${glogg_SOURCES}
    ${FileWatcherEngine_SOURCES}
    ${glogg_ITESTS}
    itests.cpp
)

target_link_libraries(glogg_itests ${LIBS} pthread Qt5::Widgets Qt5::Test)

add_executable(glogg_ptests
    ${glogg_SOURCES}
    ${FileWatcherEngine_SOURCES}
    ${glogg_PTESTS}
    itests.cpp
)

target_link_libraries(glogg_ptests ${LIBS} pthread Qt5::Widgets Qt5::Test)

add_test(
    NAME glogg_tests
    COMMAND glogg_tests
)
