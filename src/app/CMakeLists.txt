
add_executable(klogg WIN32
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/glogg.qrc

)

set_target_properties(klogg PROPERTIES AUTORCC ON)

target_link_libraries(klogg PUBLIC ui cli11)

if (WIN32)
    include(generate_product_version)

    generate_product_version(ProductVersionResourceFiles
        NAME Klogg
        ICON "${CMAKE_CURRENT_SOURCE_DIR}/glogg48.ico"
        VERSION_MAJOR 1
        VERSION_MINOR 1
        VERSION_PATCH 3
        VERSION_REVISION ${BUILD_NUMBER}
        COMPANY_NAME "Anton Filimonov"
    )

    target_sources(klogg PRIVATE ${ProductVersionResourceFiles})

endif()

install(TARGETS
    klogg
    DESTINATION bin
)
