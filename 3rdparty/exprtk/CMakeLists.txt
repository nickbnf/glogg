add_library(exprtk INTERFACE)
target_link_libraries(exprtk INTERFACE robin_hood)
target_include_directories(exprtk SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(exprtk INTERFACE 
    -Dexprtk_disable_caseinsensitivity
    -Dexprtk_disable_comments
    -Dexprtk_disable_break_continue
    -Dexprtk_disable_return_statement
    -Dexprtk_disable_superscalar_unroll
    -Dexprtk_disable_rtl_io_file
    -Dexprtk_disable_rtl_vecops)
