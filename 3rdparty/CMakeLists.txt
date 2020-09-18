add_subdirectory(cli11) #1.9.0
add_subdirectory(abseil) #20200225.2
mark_as_advanced(FORCE ABSL_RUN_TESTS)
mark_as_advanced(FORCE ABSL_STD_CXX_FLAG)
mark_as_advanced(FORCE ABSL_USE_GOOGLETEST_HEAD)
add_subdirectory(plog) #1.1.5
add_subdirectory(uchardet) #0.0.7
add_subdirectory(efsw)
set(BUILD_TEST_APP OFF CACHE INTERNAL "" FORCE)
mark_as_advanced(FORCE VERBOSE)
add_subdirectory(named_type)
add_subdirectory(catch) #v2.12.1
add_subdirectory(tbb)
mark_as_advanced(FORCE TBB_BUILD_TESTS)
mark_as_advanced(FORCE TBB_CI_BUILD)
add_subdirectory(singleapp) #3.0.15
add_subdirectory(immer)
add_subdirectory(cmzlib)
add_subdirectory(cmbzip2)
add_subdirectory(cmliblzma)
add_subdirectory(karchive)
add_subdirectory(maddy)
add_subdirectory(whereami)

set(XXHASH_BUILD_ENABLE_INLINE_API OFF) #optional
set(XXHASH_BUILD_XXHSUM OFF) #optional
add_subdirectory(xxhash/cmake_unofficial) #0.8


