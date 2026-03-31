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

qt_add_executable(EtherCATAliasGUI WIN32 ${SOURCES})

if(WIN32)
    target_sources(EtherCATAliasGUI PRIVATE app.rc)
    set_source_files_properties(app.rc PROPERTIES SKIP_AUTOMOC TRUE SKIP_AUTOUIC TRUE)
endif()

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

# ---- Install rules (used by CPack) ----
install(TARGETS EtherCATAliasGUI RUNTIME DESTINATION . COMPONENT Runtime)
install(FILES MK2_alias_config.json DESTINATION . COMPONENT Runtime)

# Deploy Qt DLLs during install using Qt's built-in deploy script.
# QT_DEPLOY_BIN_DIR defaults to "bin"; set it to "." first so DLLs
# land next to the exe instead of a bin/ subdirectory.
qt_generate_deploy_app_script(
    TARGET EtherCATAliasGUI
    OUTPUT_SCRIPT qt_deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(CODE "set(QT_DEPLOY_BIN_DIR \".\")" COMPONENT Runtime)
install(SCRIPT ${qt_deploy_script} COMPONENT Runtime)

# Install MinGW runtime DLLs (path derived from compiler location)
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    get_filename_component(MINGW_BIN_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
    foreach(dll libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll)
        install(FILES "${MINGW_BIN_DIR}/${dll}" DESTINATION . COMPONENT Runtime)
    endforeach()
endif()
