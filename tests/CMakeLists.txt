find_package(Qt5 COMPONENTS Test REQUIRED)

add_subdirectory(helpers)
add_subdirectory(unit)
add_subdirectory(ui)

add_dependencies(klogg_itests file_write_helper)
add_dependencies(ci_build klogg_tests klogg_itests)






