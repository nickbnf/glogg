#version 36ee4ef7e4d0
cmake_minimum_required(VERSION 3.2)

option (VERBOSE "Build efsw with verbose mode.")
option (BUILD_TEST_APP "Build the test app")

if (VERBOSE)
	add_definitions(-DEFSW_VERBOSE)
endif()

add_library(efsw STATIC 
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

target_include_directories(efsw PUBLIC include)
target_include_directories(efsw PRIVATE src)

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

if (APPLE)
	target_sources(efsw PRIVATE 
			src/efsw/FileWatcherKqueue.cpp
			src/efsw/WatcherKqueue.cpp)
			
	target_compile_definitions(efsw PUBLIC -DEFSW_FSEVENTS_NOT_SUPPORTED)
elseif (WIN32)
	target_sources(efsw PRIVATE 
			src/efsw/FileWatcherWin32.cpp
			src/efsw/WatcherWin32.cpp)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	target_sources(efsw PRIVATE 
			src/efsw/FileWatcherInotify.cpp
			src/efsw/WatcherInotify.cpp)

	if (NOT EXISTS "/usr/include/sys/inotify.h" AND NOT EXISTS "/usr/local/include/sys/inotify.h")
		target_compile_definitions(efsw PUBLIC -DEFSW_INOTIFY_NOSYS)
	endif()	  
endif()

if (MSVC)
	target_compile_definitions(efsw PRIVATE -D_SCL_SECURE_NO_WARNINGS)
else ()
        target_compile_options(efsw PRIVATE
            -Wall
            -Wno-long-long -Wno-variadic-macros
            -Wno-pedantic -Wno-unused-parameter -Wno-shadow
            -Wimplicit-fallthrough=0)
endif()

if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
	target_compile_definitions(efsw PRIVATE -DDEBUG)
elseif (${CMAKE_BUILD_TYPE} MATCHES "Release")
	target_compile_definitions(efsw PRIVATE -DNDEBUG)
endif()

if (APPLE)
	set(MAC_LIBS "-framework CoreFoundation" "-framework CoreServices")
	target_link_libraries(efsw ${MAC_LIBS})
elseif (NOT (${CMAKE_SYSTEM_NAME} MATCHES "Haiku") AND NOT WIN32)
	target_link_libraries(efsw pthread)
endif()	

if (BUILD_TEST_APP)
    add_executable(efsw-test src/test/efsw-test.cpp)
    target_link_libraries(efsw-test efsw)
endif()
