# EtherCAT Alias Tool

A Windows GUI application for discovering EtherCAT slaves, reading their current aliases, and writing new aliases to their EEPROM. Built with C++17, Qt 6, and [SOEM](https://github.com/OpenEtherCATsociety/SOEM).

## Features

- Lists available network adapters
- Scans and displays all EtherCAT slaves (name, vendor ID, product code, current alias, serial number)
- Reads the current alias from each slave's EEPROM
- Writes a new alias from a user-editable JSON label dictionary or a manually entered value
- Automatically recalculates the EEPROM CRC after writing

## Prerequisites

| Dependency | Notes |
|---|---|
| [Qt 6](https://www.qt.io/download) | Install with the MinGW 64-bit component |
| [Npcap](https://npcap.com/) | Required for raw Ethernet socket access on Windows. Install with "WinPcap API-compatible mode" enabled. |
| CMake ≥ 3.16 | Included with Qt installer under `Tools/CMake_64` |
| Ninja | Included with Qt installer under `Tools/Ninja` |

## Cloning

This repository uses SOEM as a git submodule. Clone with:

```bash
git clone --recurse-submodules <repo-url>
```

If you already cloned without `--recurse-submodules`:

```bash
git submodule update --init
```

## Building

Replace `<Qt6-install-path>` with your Qt 6 MinGW installation directory (e.g. `C:/Qt/6.11.0/mingw_64`) and `<MinGW-path>` with the matching MinGW toolchain (e.g. `C:/Qt/Tools/mingw1310_64`).

```bash
cmake -B build \
  -G "Ninja" \
  -DCMAKE_PREFIX_PATH="<Qt6-install-path>" \
  -DCMAKE_C_COMPILER="<MinGW-path>/bin/gcc.exe" \
  -DCMAKE_CXX_COMPILER="<MinGW-path>/bin/g++.exe" \
  -DCMAKE_MAKE_PROGRAM="<Qt6-install-path>/../../../Tools/Ninja/ninja.exe" \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build
```

The executable and all required Qt DLLs will be placed in `build/gui/`.

> **Note:** Add `<MinGW-path>/bin` to your `PATH` before running cmake if the compiler test fails.

## Running

Raw Ethernet access requires Administrator privileges. Right-click the executable and select **Run as administrator**, or from an elevated terminal:

```powershell
Start-Process -Verb RunAs build\gui\EtherCATAliasGUI.exe
```

## Alias Configuration

The file `MK2_alias_config.json` (placed next to the executable after build) maps human-readable joint labels to alias values:

```json
{
  "LSP": 3101,
  "RSP": 3201,
  "TY":  3001
}
```

Edit this file directly to add or change labels, then click **Reload Config…** in the app. No recompile needed.

## Usage

1. Select a network adapter from the dropdown.
2. Click **Scan** to discover slaves.
3. Select a slave in the table.
4. Pick a label from the **From config** dropdown, or type a decimal alias value manually.
5. Click **Write Alias to Selected Slave**.
6. Power-cycle the slave for the new alias to take effect.
