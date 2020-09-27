add_executable(klogg_itests
    logdata_test.cpp
    logfiltereddata_test.cpp
    crawlerwidget_test.cpp
    mainwindow_test.cpp
    qtests_main.cpp
)

target_link_libraries(klogg_itests ui catch Qt5::Test test_utils)
set_target_properties(klogg_itests PROPERTIES AUTOMOC ON)

if (${USE_LTO})
set_property(TARGET klogg_itests PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

add_test(
    NAME klogg_itests
    COMMAND klogg_itests -platform offscreen
)