add_library(logdata STATIC
         ${CMAKE_CURRENT_SOURCE_DIR}/include/data/abstractlogdata.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/data/blockpool.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/data/compressedlinestorage.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/data/encodingdetector.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/data/linepositionarray.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/data/loadingstatus.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/data/logdata.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/data/logdataoperation.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/data/logdataworker.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/data/logfiltereddata.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/data/logfiltereddataworker.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/data/linetypes.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/data/fileholder.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/data/filedigest.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/data/hsregularexpression.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/data/regularexpression.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/data/regularexpressionpattern.h



        ${CMAKE_CURRENT_SOURCE_DIR}/src/abstractlogdata.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/blockpool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/compressedlinestorage.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/encodingdetector.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/logdata.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/logdataoperation.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/logdataworker.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/logfiltereddata.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/logfiltereddataworker.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/fileholder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/filedigest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/hsregularexpression.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/regularexpression.cpp

        src/filedigest.cpp)

set_target_properties(logdata PROPERTIES AUTOMOC ON)

target_include_directories(logdata PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(logdata PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/data")

target_link_libraries(logdata PUBLIC
        Qt5::Core
        roaring
        tbb_wrapper
        named_type
        utils
        filewatch
        uchardet_wrapper
        exprtk)

target_link_libraries(logdata PRIVATE xxhash_wrapper)

if(KLOGG_USE_HYPERSCAN)
target_link_libraries(logdata PUBLIC hypersan_wrapper)
target_compile_definitions(logdata PUBLIC KLOGG_HAS_HS)
endif()



if (${KLOGG_USE_LTO})
set_property(TARGET logdata PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
