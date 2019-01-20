add_library(ui STATIC
         ${CMAKE_CURRENT_SOURCE_DIR}/include/abstractlogview.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/crawlerwidget.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/filteredview.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/filtersdialog.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/filterset.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/infoline.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/logmainview.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/mainwindow.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/menuactiontooltipbehavior.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/optionsdialog.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/overview.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/overviewwidget.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/qfnotifications.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/quickfind.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/quickfindmux.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/quickfindpattern.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/quickfindwidget.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/recentfiles.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/savedsearches.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/selection.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/session.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/sessioninfo.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/signalmux.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/tabbedcrawlerwidget.h
         #${CMAKE_CURRENT_SOURCE_DIR}/include/versionchecker.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/viewinterface.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/viewtools.h

        ${CMAKE_CURRENT_SOURCE_DIR}/include/filtersdialog.ui
        ${CMAKE_CURRENT_SOURCE_DIR}/include/optionsdialog.ui

        ${CMAKE_CURRENT_SOURCE_DIR}/src/abstractlogview.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/crawlerwidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/filteredview.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/filtersdialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/filterset.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/infoline.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/logmainview.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/mainwindow.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/menuactiontooltipbehavior.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/optionsdialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/overview.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/overviewwidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/quickfind.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/quickfindmux.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/quickfindpattern.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/quickfindwidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/recentfiles.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/savedsearches.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/selection.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/session.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/sessioninfo.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/signalmux.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/tabbedcrawlerwidget.cpp
        #${CMAKE_CURRENT_SOURCE_DIR}/src/versionchecker.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/viewtools.cpp
)

set_target_properties(ui PROPERTIES AUTOUIC ON)
set_property(TARGET ui APPEND PROPERTY AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/ui")

target_include_directories(ui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(ui PUBLIC Qt5::Widgets logdata externalcom absl::any)

add_dependencies(ui generate_version)

if (WIN32)
    target_link_libraries(ui PUBLIC user32)
endif()
