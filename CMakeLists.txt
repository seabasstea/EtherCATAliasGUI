cmake_minimum_required(VERSION 3.16)
project(EtherCATAliasGUI VERSION 1.0 LANGUAGES C CXX)

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

# Only package the Runtime component — excludes SOEM's dev files (headers, lib, cmake)
set(CPACK_COMPONENTS_ALL Runtime)

include(CPack)
