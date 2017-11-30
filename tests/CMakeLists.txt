find_package(Qt5Test REQUIRED)

# Add test cpp file
add_executable(glogg_tests
    linepositionarrayTest.cpp
    #watchtowerTest.cpp
)

set(TEST_LIBS ui gtest Qt5::Test)

target_link_libraries(glogg_tests ${TEST_LIBS})

add_executable(file_write_helper
	file_write_helper.cpp
	file_write_helper.h
)

target_link_libraries(file_write_helper ${TEST_LIBS})

add_executable(glogg_itests
    logdataTest.cpp
    logfiltereddataTest.cpp
    itests.cpp
)

target_link_libraries(glogg_itests ${TEST_LIBS})

add_dependencies(glogg_itests file_write_helper)

add_executable(glogg_ptests
    logdataPerfTest.cpp
    logfiltereddataPerfTest.cpp
    itests.cpp
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
