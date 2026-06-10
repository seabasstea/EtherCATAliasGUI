cmake_minimum_required(VERSION 3.16)

# CI overrides this from the git tag (v1.2 -> -DAPP_VERSION=1.2) so release
# artifacts and the in-app update check always match the tag.
set(APP_VERSION "1.1" CACHE STRING "Application version")
project(EtherCATAliasGUI VERSION ${APP_VERSION} LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(SOEM)
add_subdirectory(gui)

# ---- CPack installer configuration ----
set(CPACK_PACKAGE_NAME               "EtherCATAliasTool")
set(CPACK_PACKAGE_VENDOR             "Novanta")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "EtherCAT Alias Configuration Tool")
set(CPACK_PACKAGE_VERSION            ${PROJECT_VERSION})
set(CPACK_PACKAGE_INSTALL_DIRECTORY  "EtherCAT Alias Tool")
set(CPACK_PACKAGE_EXECUTABLES        "EtherCATAliasGUI" "EtherCAT Alias Tool")
set(CPACK_PACKAGE_CONTACT            "Novanta <sebastian.sanchez@foundation.bot>") # DEB requires a maintainer

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME         "EtherCAT Alias Tool")
    set(CPACK_NSIS_PACKAGE_NAME         "EtherCAT Alias Tool")
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN   "EtherCATAliasGUI.exe")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)

    # Start Menu shortcut
    set(CPACK_NSIS_CREATE_ICONS_EXTRA
        "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\EtherCAT Alias Tool.lnk' '$INSTDIR\\\\EtherCATAliasGUI.exe'")
    set(CPACK_NSIS_DELETE_ICONS_EXTRA
        "Delete '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\EtherCAT Alias Tool.lnk'")

    # Npcap prerequisite check — abort if not installed
    set(CPACK_NSIS_EXTRA_PREINSTALL_COMMANDS
        "IfFileExists $SYSDIR\\\\Npcap\\\\wpcap.dll npcap_ok npcap_abort\nnpcap_abort:\nMessageBox MB_OK|MB_ICONEXCLAMATION 'Npcap is required but not installed. Please download and install Npcap from https://npcap.com, then run this installer again.'\nAbort\nnpcap_ok:")
else()
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_NAME "ethercat-alias-tool")    # Debian package names must be lowercase
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)                 # ethercat-alias-tool_<version>_<arch>.deb
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)                  # auto-detect libqt6widgets6, libc6, ...
    # The xcb platform plugin is dlopened, invisible to shlibdeps; libcap2-bin provides setcap for postinst
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "qt6-qpa-plugins, libcap2-bin")
    set(CPACK_DEBIAN_PACKAGE_SECTION "electronics")
    set(CPACK_DEB_COMPONENT_INSTALL ON)                     # honor CPACK_COMPONENTS_ALL (keeps SOEM dev files out)
    set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)    # single .deb without a "-runtime" suffix
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/gui/packaging/postinst")
endif()

# Only package the Runtime component — excludes SOEM's dev files (headers, lib, cmake)
set(CPACK_COMPONENTS_ALL Runtime)

include(CPack)
