find_package(Qt5Test REQUIRED)

# Add test cpp file
add_executable(klogg_tests
    linepositionarrayTest.cpp
    tests_main.cpp
)

set(TEST_LIBS ui catch Qt5::Test)

target_link_libraries(klogg_tests ${TEST_LIBS})
set_target_properties(klogg_tests PROPERTIES AUTOMOC ON)


if (${USE_LTO})
set_property(TARGET klogg_tests PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

add_executable(file_write_helper
        file_write_helper.cpp
        file_write_helper.h
)

target_link_libraries(file_write_helper ${TEST_LIBS})

add_executable(klogg_itests
    logdataTest.cpp
    logfiltereddataTest.cpp
    qtests_main.cpp
)

target_link_libraries(klogg_itests ${TEST_LIBS})
set_target_properties(klogg_itests PROPERTIES AUTOMOC ON)

add_dependencies(klogg_itests file_write_helper)
if (${USE_LTO})
set_property(TARGET klogg_itests PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()


add_test(
    NAME klogg_tests
    COMMAND klogg_tests -platform offscreen
)

add_test(
    NAME klogg_itests
    COMMAND klogg_itests -platform offscreen
)

if (ENABLE_COVERAGE)
    SETUP_TARGET_FOR_COVERAGE(
        NAME glogg_itests_cov
        EXECUTABLE klogg_tests
        DEPENDENCIES klogg_itests)
endif()
