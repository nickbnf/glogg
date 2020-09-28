set(UI_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/qtests_main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/logdata_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/logfiltereddata_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/crawlerwidget_test.cpp
)

if(NOT APPLE)
    LIST(APPEND UI_TEST_SOURCES  ${CMAKE_CURRENT_SOURCE_DIR}/mainwindow_test.cpp)
endif()

add_executable(klogg_itests ${UI_TEST_SOURCES})

target_link_libraries(klogg_itests ui catch Qt5::Test test_utils)
set_target_properties(klogg_itests PROPERTIES AUTOMOC ON)

if (${USE_LTO})
set_property(TARGET klogg_itests PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

add_test(
    NAME klogg_itests
    COMMAND klogg_itests -platform offscreen
)