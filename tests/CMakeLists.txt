INCLUDE (CheckSymbolExists)
INCLUDE (CheckCXXSourceCompiles)
INCLUDE (CheckCXXCompilerFlag)

cmake_minimum_required(VERSION 2.8.12)
project(glogg_tests)

# Qt
find_program(QT_QMAKE_EXECUTABLE NAMES qmake PATHS $ENV{QT_DIR}/bin)
find_package(Qt5Widgets REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

check_cxx_compiler_flag(-Wall HAS_WALL)

if(HAS_WALL)
    set(PARANOID_FLAGS "${PARANOID_FLAGS} -Wall")
endif(HAS_WALL)

# Configuration
check_symbol_exists( symlink "unistd.h" HAVE_SYMLINK )

check_cxx_source_compiles(CMAKE_REQUIRED_FLAGS="-std=c++11" "
    #include <memory>
    int main(int, int)
    {
        auto test = std::make_unique<void>();
        return 0;
    }
" HAVE_MAKE_UNIQUE)

configure_file(../config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

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
    ../src/qtfilewatcher.cpp
    ../src/tabbedcrawlerwidget.cpp
    ../src/filewatcher.cpp
    ../src/watchtowerlist.cpp
    ../src/watchtower.cpp
    ../src/platformfilewatcher.cpp
)

set(glogg_HEADERS
    ../src/mainwindow.h
    ../src/marks.h
    ../src/filewatcher.h)

set(glogg_TESTS
    testtest.cpp
    filewatcherTest.cpp
    watchtowerTest.cpp
)

# Options
if (WIN32)
    set(FileWatcherEngine_SOURCES
        ../src/winwatchtowerdriver.cpp
    )
else (WIN32)
    set(FileWatcherEngine_SOURCES ../src/inotifywatchtowerdriver.cpp)
endif (WIN32)

# Qt
# QT4_WRAP_CPP(glogg_HEADERS_MOC ${glogg_HEADERS})
# INCLUDE(${QT_USE_FILE})
# ADD_DEFINITIONS(${QT_DEFINITIONS})
set(CMAKE_CXX_FLAGS "${PARANOID_FLAGS} -fPIC -std=c++11 -DGLOGG_VERSION=\\\"unit_tests\\\" -g -gdwarf-2")

# Add test cpp file
add_executable(glogg_tests
    ${glogg_SOURCES}
    ${FileWatcherEngine_SOURCES}
    ${glogg_HEADERS_MOC}
    ${glogg_TESTS}
)

# Link test executable against gtest & gtest_main
target_link_libraries(glogg_tests gmock gtest gtest_main pthread Qt5::Widgets)

add_test(
    NAME glogg_tests
    COMMAND glogg_tests
)
