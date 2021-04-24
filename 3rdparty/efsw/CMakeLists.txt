cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

project (efsw)

include(GNUInstallDirs)

find_package(Threads REQUIRED)

option (VERBOSE "Build efsw with verbose mode.")
option (BUILD_SHARED_LIBS "Build efsw as a shared library" ON)
option (BUILD_TEST_APP "Build the test app")
option (EFSW_INSTALL "Add efsw install targets" ON)

add_library(efsw)

target_sources(efsw PRIVATE
src/efsw/Debug.cpp
	src/efsw/DirectorySnapshot.cpp
	src/efsw/DirectorySnapshotDiff.cpp
	src/efsw/DirWatcherGeneric.cpp
	src/efsw/FileInfo.cpp
	src/efsw/FileSystem.cpp
	src/efsw/FileWatcher.cpp
	src/efsw/FileWatcherCWrapper.cpp
	src/efsw/FileWatcherGeneric.cpp
	src/efsw/FileWatcherImpl.cpp
	src/efsw/Log.cpp
	src/efsw/Mutex.cpp
	src/efsw/String.cpp
	src/efsw/System.cpp
	src/efsw/Thread.cpp
	src/efsw/Watcher.cpp
	src/efsw/WatcherGeneric.cpp
)

target_include_directories(efsw
	PRIVATE src/
	PUBLIC 
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if (VERBOSE)
	target_compile_definitions(efsw PRIVATE EFSW_VERBOSE)
endif()
if (BUILD_SHARED_LIBS)
	target_compile_definitions(efsw PRIVATE EFSW_DYNAMIC EFSW_EXPORTS)
endif()

# platforms
if (WIN32)
	target_sources(efsw PRIVATE
		src/efsw/platform/win/FileSystemImpl.cpp
		src/efsw/platform/win/MutexImpl.cpp
		src/efsw/platform/win/SystemImpl.cpp
		src/efsw/platform/win/ThreadImpl.cpp
	)
else ()
	target_sources(efsw PRIVATE
		src/efsw/platform/posix/FileSystemImpl.cpp
		src/efsw/platform/posix/MutexImpl.cpp
		src/efsw/platform/posix/SystemImpl.cpp
		src/efsw/platform/posix/ThreadImpl.cpp
	)
endif()

# watcher implementations

if (APPLE)
	target_sources(efsw PRIVATE
		src/efsw/FileWatcherFSEvents.cpp
		src/efsw/FileWatcherKqueue.cpp
		src/efsw/WatcherFSEvents.cpp
		src/efsw/WatcherKqueue.cpp
	)

	if (NOT CMAKE_SYSTEM_VERSION GREATER 9)
		target_compile_definitions(efsw PRIVATE EFSW_FSEVENTS_NOT_SUPPORTED)
	endif()
elseif (WIN32)
	target_sources(efsw PRIVATE
		src/efsw/FileWatcherWin32.cpp
		src/efsw/WatcherWin32.cpp
	)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	target_sources(efsw PRIVATE
		src/efsw/FileWatcherInotify.cpp
		src/efsw/WatcherInotify.cpp
	)

	if (NOT EXISTS "/usr/include/sys/inotify.h" AND NOT EXISTS "/usr/local/include/sys/inotify.h")
		target_compile_definitions(efsw PRIVATE EFSW_INOTIFY_NOSYS)
	endif()
elseif (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
	target_sources(efsw PRIVATE
		src/efsw/FileWatcherKqueue.cpp
		src/efsw/WatcherKqueue.cpp
	)
endif()

if (MSVC)
	target_compile_definitions(efsw PRIVATE _SCL_SECURE_NO_WARNINGS)
else ()
	target_compile_options(efsw PRIVATE -Wall -Wno-long-long)
endif()

if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
	target_compile_definitions(efsw PRIVATE DEBUG)
elseif (${CMAKE_BUILD_TYPE} MATCHES "Release")
	target_compile_definitions(efsw PRIVATE NDEBUG)
endif()

if (APPLE)
	set(MAC_LIBS "-framework CoreFoundation" "-framework CoreServices")
	target_link_libraries(efsw PRIVATE ${MAC_LIBS})
elseif (NOT (${CMAKE_SYSTEM_NAME} MATCHES "Haiku") AND NOT WIN32)
	target_link_libraries(efsw PRIVATE Threads::Threads)
endif()

include(CMakePackageConfigHelpers)

set(packageDestDir "${CMAKE_INSTALL_LIBDIR}/cmake/${CMAKE_PROJECT_NAME}")

configure_package_config_file(
	${CMAKE_CURRENT_SOURCE_DIR}/efswConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/cmake/efswConfig.cmake
	INSTALL_DESTINATION "${packageDestDir}"
	NO_SET_AND_CHECK_MACRO
	NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

export(TARGETS efsw NAMESPACE efsw:: FILE ${CMAKE_CURRENT_BINARY_DIR}/cmake/${CMAKE_PROJECT_NAME}Targets.cmake)

if (EFSW_INSTALL)
  install(TARGETS efsw EXPORT efswExport
  	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

  install (
  	FILES
  		include/efsw/efsw.h include/efsw/efsw.hpp
  		src/efsw/base.hpp src/efsw/System.hpp src/efsw/FileSystem.hpp
  	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/efsw
  )
  
  install(EXPORT efswExport NAMESPACE efsw:: DESTINATION "${packageDestDir}" FILE ${CMAKE_PROJECT_NAME}Targets.cmake)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cmake/efswConfig.cmake DESTINATION "${packageDestDir}")
endif()

if (BUILD_TEST_APP)
	add_executable(efsw-test src/test/efsw-test.cpp)
	target_link_libraries(efsw-test efsw)
endif()

