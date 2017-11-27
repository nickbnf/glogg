
if (APPLE)
    set(OS_BUNDLE MACOSX_BUNDLE)
elseif (WIN32)
    set(OS_BUNDLE WIN32)
endif()

add_executable(klogg ${OS_BUNDLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/glogg.qrc
    ${ICON_FILE}
)

set_target_properties(klogg PROPERTIES AUTORCC ON)

target_link_libraries(klogg PUBLIC ui cli11)

if (WIN32)
    include(win32_rc)

    generate_product_version(
        ProductVersionResourceFiles
        NAME ${PROJECT_NAME}
        ICON "${ICON_FILE}"
        VERSION_MAJOR ${PRODUC_VERSION_MAJOR}
        VERSION_MINOR ${PRODUC_VERSION_MINOR}
        VERSION_PATCH ${PRODUC_VERSION_PATCH}
        VERSION_REVISION ${PRODUC_VERSION_TWEAK}
        COMPANY_NAME ${COMPANY}
    )

    target_sources(klogg PRIVATE ${ProductVersionResourceFiles})

elseif (APPLE)
    set_source_files_properties(${ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

    set(MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME})
    set(MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
    set(MACOSX_BUNDLE_LONG_VERSION_STRING ${PROJECT_VERSION})
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
    set(MACOSX_BUNDLE_COPYRIGHT ${COPYRIGHT})
    set(MACOSX_BUNDLE_GUI_IDENTIFIER ${IDENTIFIER})
    set(MACOSX_BUNDLE_ICON_FILE ${ICON_NAME})

endif()

install(TARGETS
    klogg
    DESTINATION bin
)
