# Add test cpp file
add_executable(klogg_tests
    linepositionarray_test.cpp
    tests_main.cpp
)

target_link_libraries(klogg_tests ui catch Qt5::Test)
set_target_properties(klogg_tests PROPERTIES AUTOMOC ON)

if (${USE_LTO})
set_property(TARGET klogg_tests PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

add_test(
    NAME klogg_tests
    COMMAND klogg_tests -platform offscreen
)