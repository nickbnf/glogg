add_library(stlab INTERFACE)
target_include_directories(stlab SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(stlab INTERFACE absl::optional absl::variant)
target_compile_definitions(stlab INTERFACE -DSTLAB_DISABLE_FUTURE_COROUTINES)
 
