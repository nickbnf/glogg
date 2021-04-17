add_subdirectory(cli11) #1.9.0
add_subdirectory(plog) #1.1.5
add_subdirectory(uchardet) #0.0.7
add_subdirectory(named_type)
add_subdirectory(catch) #v2.12.1

set(ABSL_ENABLE_INSTALL OFF CACHE INTERNAL "" FORCE)
add_subdirectory(abseil) #20210324.0
mark_as_advanced(FORCE ABSL_RUN_TESTS)
mark_as_advanced(FORCE ABSL_STD_CXX_FLAG)
mark_as_advanced(FORCE ABSL_USE_GOOGLETEST_HEAD)

add_subdirectory(efsw)
set(BUILD_TEST_APP OFF CACHE INTERNAL "" FORCE)
mark_as_advanced(FORCE VERBOSE)

set(TBB_TEST OFF CACHE INTERNAL "" FORCE)
set(TBB_EXAMPLES OFF CACHE INTERNAL "" FORCE)
set(TBB_STRICT OFF CACHE INTERNAL "" FORCE)
add_subdirectory(onetbb) #2021.2.0
mark_as_advanced(FORCE TBB_TEST)
mark_as_advanced(FORCE TBB_EXAMPLES)
mark_as_advanced(FORCE TBB_STRICT)
mark_as_advanced(FORCE TBB_WINDOWS_DRIVER)
mark_as_advanced(FORCE TBB_NO_APPCONTAINER)
mark_as_advanced(FORCE TBB4PY_BUILD)
mark_as_advanced(FORCE TBB_CPF)
mark_as_advanced(FORCE TBB_FIND_PACKAGE)

target_compile_definitions(klogg_tbb INTERFACE __TBB_NO_IMPLICIT_LINKAGE=1)

add_subdirectory(singleapp) #3.1.5
add_subdirectory(immer)
add_subdirectory(cmzlib)
add_subdirectory(cmbzip2)
add_subdirectory(cmliblzma)
add_subdirectory(karchive)
add_subdirectory(maddy)
add_subdirectory(whereami)

add_subdirectory(hyperscan) #5.4.0

if(KLOGG_USE_SENTRY)
    set(SENTRY_BACKEND "crashpad" CACHE INTERNAL "" FORCE)
    set(SENTRY_TRANSPORT "none" CACHE INTERNAL "" FORCE)
    set(SENTRY_BUILD_EXAMPLES OFF CACHE INTERNAL "" FORCE)
    set(CRASHPAD_ENABLE_INSTALL OFF CACHE INTERNAL "" FORCE)
    set(CRASHPAD_ENABLE_INSTALL_DEV OFF CACHE INTERNAL "" FORCE)
    add_subdirectory(sentry)
endif(KLOGG_USE_SENTRY)

set(XXHASH_BUILD_ENABLE_INLINE_API OFF) #optional
set(XXHASH_BUILD_XXHSUM OFF) #optional
add_subdirectory(xxhash/cmake_unofficial) #0.8


