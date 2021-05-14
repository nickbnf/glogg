add_subdirectory(cli11) #1.9.0
add_subdirectory(plog) #1.1.5
add_subdirectory(named_type)
add_subdirectory(catch) #v2.12.1
add_subdirectory(singleapp) #3.1.5
add_subdirectory(cmzlib)
add_subdirectory(cmbzip2)
add_subdirectory(cmliblzma)
add_subdirectory(karchive)
add_subdirectory(maddy)
add_subdirectory(whereami)
add_subdirectory(kdab)
add_subdirectory(roaring) #0.3.1

if(KLOGG_USE_SENTRY)
    set(SENTRY_BACKEND "crashpad" CACHE INTERNAL "" FORCE)
    set(SENTRY_TRANSPORT "none" CACHE INTERNAL "" FORCE)
    set(SENTRY_BUILD_EXAMPLES OFF CACHE INTERNAL "" FORCE)
    set(CRASHPAD_ENABLE_INSTALL OFF CACHE INTERNAL "" FORCE)
    set(CRASHPAD_ENABLE_INSTALL_DEV OFF CACHE INTERNAL "" FORCE)
    add_subdirectory(sentry)
endif(KLOGG_USE_SENTRY)

set(EFSW_INSTALL OFF CACHE INTERNAL "" FORCE)
add_subdirectory(efsw) #2bfda30 on 14 Dec 2020
set(BUILD_TEST_APP OFF CACHE INTERNAL "" FORCE)
mark_as_advanced(FORCE VERBOSE)

#ABSEIL
add_library(absl_wrapper INTERFACE)
pkg_check_modules(ABSL_VARIANT absl_variant)
pkg_check_modules(ABSL_OPTIONAL absl_optional)

if(ABSL_VARIANT_FOUND AND ABSL_OPTIONAL_FOUND)
    message("Using system abseil")

    target_include_directories(absl_wrapper SYSTEM INTERFACE ${ABSL_VARIANT_INCLUDE_DIRS})
    target_include_directories(absl_wrapper SYSTEM INTERFACE ${ABSL_OPTIONAL_INCLUDE_DIRS})

    target_link_libraries(absl_wrapper INTERFACE ${ABSL_VARIANT_LINK_LIBRARIES})
    target_link_libraries(absl_wrapper INTERFACE ${ABSL_OPTIONAL_LINK_LIBRARIES})
else()
    set(ABSL_ENABLE_INSTALL OFF CACHE INTERNAL "" FORCE)
    add_subdirectory(abseil) #20210324.0
    mark_as_advanced(FORCE ABSL_RUN_TESTS)
    mark_as_advanced(FORCE ABSL_STD_CXX_FLAG)
    mark_as_advanced(FORCE ABSL_USE_GOOGLETEST_HEAD)

    target_link_libraries(absl_wrapper INTERFACE 
        absl::variant 
        absl::optional 
        absl::synchronization)

endif()

#TBB
add_library(tbb_wrapper INTERFACE)
add_library(tbbmalloc_proxy_wrapper INTERFACE)

pkg_check_modules(TBB tbb>=2021)
pkg_check_modules(TBB_MALLOC_PROXY tbbmalloc_proxy>=2021)
if (TBB_FOUND AND TBB_MALLOC_PROXY_FOUND)
    message("Using system tbb")
    target_include_directories(tbb_wrapper SYSTEM INTERFACE ${TBB_INCLUDE_DIRS})
    target_include_directories(tbb_malloc_proxy_wrapper SYSTEM INTERFACE ${TBB_MALLOC_PROXY_INCLUDE_DIRS})

    target_link_libraries(tbb_wrapper INTERFACE ${TBB_LINK_LIBRARIES})
    target_link_libraries(tbbmalloc_proxy_wrapper INTERFACE ${TBB_MALLOC_PROXY_LINK_LIBRARIES})
else()
    message("Using bundled tbb")
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

    target_link_libraries(tbb_wrapper INTERFACE klogg_tbb)
    target_link_libraries(tbbmalloc_proxy_wrapper INTERFACE klogg_tbbmalloc_proxy)
endif()

#UCHARDET
add_library(uchardet_wrapper INTERFACE)
pkg_check_modules(UCHARDET uchardet>=0.0.7)
if(UCHARDET_FOUND)
    message("Using system uchardet")
    target_include_directories(uchardet_wrapper SYSTEM INTERFACE ${UCHARDET_INCLUDE_DIRS})
    target_link_libraries(uchardet_wrapper INTERFACE ${UCHARDET_LINK_LIBRARIES})
else()
    message("Using bundled uchardet")
    add_subdirectory(uchardet) #0.0.7
    target_link_libraries(uchardet_wrapper INTERFACE uchardet)
endif()

#HYPERSCAN
if(KLOGG_USE_HYPERSCAN)
add_library(hypersan_wrapper INTERFACE)
pkg_check_modules(HYPESCAN libhs>=5.4)
if(HYPESCAN_FOUND)
    message("Using system hyperscan")
    target_include_directories(hypersan_wrapper SYSTEM INTERFACE ${HYPESCAN_INCLUDE_DIRS})
    target_link_libraries(hypersan_wrapper INTERFACE ${HYPESCAN_LINK_LIBRARIES})
else(NOT Hyperscan_FOUND)
    message("Using bundled hyperscan")
    set(BUILD_EXAMPLES OFF CACHE INTERNAL "" FORCE)
    add_subdirectory(hyperscan) #5.4.0
    target_link_libraries(hypersan_wrapper INTERFACE hs)
endif()
endif(KLOGG_USE_HYPERSCAN)

#XXHASH
add_library(xxhash_wrapper INTERFACE)
pkg_check_modules(XXHASH libxxhash>=0.8)
if(XXHASH_FOUND)
    message("Using system xxhash")
    target_include_directories(xxhash_wrapper SYSTEM INTERFACE ${XXHASH_INCLUDE_DIRS})
    target_link_libraries(xxhash_wrapper INTERFACE ${XXHASH_LINK_LIBRARIES})
else()
    message("Using bundled xxhash")
    set(XXHASH_BUILD_ENABLE_INLINE_API OFF) #optional
    set(XXHASH_BUILD_XXHSUM OFF) #optional
    add_subdirectory(xxhash/cmake_unofficial) #0.8
    target_link_libraries(xxhash_wrapper INTERFACE xxHash::xxhash)
endif()


