cmake_minimum_required(VERSION 3.16)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

set(SOURCES
    main.cpp
    MainWindow.h
    MainWindow.cpp
    EtherCATWorker.h
    EtherCATWorker.cpp
    AliasConfig.h
    AliasConfig.cpp
)

qt_add_executable(EtherCATAliasGUI ${SOURCES})

target_link_libraries(EtherCATAliasGUI PRIVATE soem Qt6::Widgets)

# Copy alias_config.json next to the executable at build time
add_custom_command(TARGET EtherCATAliasGUI POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/MK2_alias_config.json"
        "$<TARGET_FILE_DIR:EtherCATAliasGUI>/MK2_alias_config.json"
    COMMENT "Copying alias_config.json"
)

# Deploy Qt DLLs on Windows (requires windeployqt in PATH)
if(WIN32)
    find_program(WINDEPLOYQT windeployqt HINTS "${Qt6_DIR}/../../../bin")
    if(WINDEPLOYQT)
        add_custom_command(TARGET EtherCATAliasGUI POST_BUILD
            COMMAND "${WINDEPLOYQT}"
                --no-translations
                --no-system-d3d-compiler
                "$<TARGET_FILE:EtherCATAliasGUI>"
            COMMENT "Running windeployqt"
        )
    endif()
endif()
