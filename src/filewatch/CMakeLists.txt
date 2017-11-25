add_library(filewatch STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/filewatcher.h

)

if (${PREFER_PLATFORM_FILEWATCH})

    include(CheckIncludeFiles)
    include(CheckSymbolExists)

    check_include_files(sys/inotify.h HAVE_SYS_INOTIFY)
    check_symbol_exists( symlink "unistd.h" HAVE_SYMLINK )

    if(WIN32)
        message("Will use windows native file watch")

        set(HAVE_PLATFORM_FILEWATCH ON)

        target_sources(filewatch PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include/winfilewatcher.h
            ${CMAKE_CURRENT_SOURCE_DIR}/include/winwatchtowerdriver.h
            ${CMAKE_CURRENT_SOURCE_DIR}/src/winfilewatcher.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/winwatchtowerdriver.cpp
        )

        target_compile_definitions(filewatch PUBLIC -DGLOGG_SUPPORTS_POLLING)
        target_compile_definitions(filewatch PUBLIC -DGLOGG_SUPPORTS_WINWATCH)

    elseif(${HAVE_SYS_INOTIFY})
        message("Will use inotify file watch")

        set(HAVE_PLATFORM_FILEWATCH ON)
        target_sources(filewatch PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include/inotifywatchtowerdriver.h
            ${CMAKE_CURRENT_SOURCE_DIR}/src/inotifywatchtowerdriver.cpp
        )
        target_compile_definitions(filewatch PUBLIC -DGLOGG_SUPPORTS_INOTIFY)
        if(${HAVE_SYMLINK})
            target_compile_definitions(filewatch PUBLIC -DGLOGG_SUPPORTS_SYMLINK)
        endif()
    endif()
endif()

if (${HAVE_PLATFORM_FILEWATCH})
    target_sources(filewatch PRIVATE
         ${CMAKE_CURRENT_SOURCE_DIR}/include/platformfilewatcher.h
         ${CMAKE_CURRENT_SOURCE_DIR}/src/platformfilewatcher.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/include/watchtower.h
         ${CMAKE_CURRENT_SOURCE_DIR}/src/watchtower.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/include/watchtowerlist.h
         ${CMAKE_CURRENT_SOURCE_DIR}/src/watchtowerlist.cpp
    )
else()
    message("Will use qt file watch")

    target_sources(filewatch PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/qtfilewatcher.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/qtfilewatcher.cpp
    )

    target_compile_definitions(filewatch PUBLIC -DGLOGG_USES_QTFILEWATCHER)
endif()

target_include_directories(filewatch PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(filewatch PUBLIC Qt5::Core utils)

