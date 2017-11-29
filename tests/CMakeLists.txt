find_package(Qt5Test REQUIRED)

# Add test cpp file
add_executable(glogg_tests
    linepositionarrayTest.cpp
    tests_main.cpp
)

set(TEST_LIBS ui gtest Qt5::Test)

target_link_libraries(glogg_tests ${TEST_LIBS})

add_executable(glogg_itests
    logdataTest.cpp
    logfiltereddataTest.cpp
    qtests_main.cpp
)

target_link_libraries(glogg_itests ${TEST_LIBS})

add_executable(glogg_ptests
    logdataPerfTest.cpp
    logfiltereddataPerfTest.cpp
    qtests_main.cpp
)

target_link_libraries(glogg_ptests ${TEST_LIBS})

add_test(
    NAME glogg_tests
    COMMAND glogg_tests
)

add_test(
    NAME glogg_itests
    COMMAND glogg_itests -platform offscreen
)
