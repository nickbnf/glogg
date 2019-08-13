add_library(qtpromise STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/QtPromise)

target_include_directories(qtpromise SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(qtpromise SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
