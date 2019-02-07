find_package(Qt5Test REQUIRED)

# Add test cpp file
add_executable(glogg_tests
    linepositionarrayTest.cpp
    tests_main.cpp
)

set(TEST_LIBS ui catch Qt5::Test)

target_link_libraries(glogg_tests ${TEST_LIBS})
set_property(TARGET glogg_tests PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

add_executable(file_write_helper
        file_write_helper.cpp
        file_write_helper.h
)

target_link_libraries(file_write_helper ${TEST_LIBS})

add_executable(glogg_itests
    logdataTest.cpp
    logfiltereddataTest.cpp
    qtests_main.cpp
)

target_link_libraries(glogg_itests ${TEST_LIBS})

add_dependencies(glogg_itests file_write_helper)
set_property(TARGET glogg_itests PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)


add_test(
    NAME glogg_tests
    COMMAND glogg_tests
)

add_test(
    NAME glogg_itests
    COMMAND glogg_itests -platform offscreen
)

if (ENABLE_COVERAGE)
    SETUP_TARGET_FOR_COVERAGE(
        NAME glogg_itests_cov
        EXECUTABLE glogg_itests
        DEPENDENCIES glogg_itests)
endif()
