# FitlyzerC

## Overview

FitlyzerC is a cross-platform desktop application for analyzing Garmin FIT activity files. It is written in modern C++23 and uses Qt 6 for its user interface, charting, networking, and database functionality.

The application is designed for endurance athletes, coaches, and performance analysts who need a native desktop tool for inspecting training sessions, intervals, power data, heart rate data, and GPS tracks.

---

## Key Features

### Activity Analysis
- Import Garmin FIT files
- Decode FIT records using the Garmin FIT SDK
- Analyze power, heart rate, cadence, speed, elevation, and GPS data
- Interactive charts with zooming and panning
- Interval inspection and workout review
- Track visualization on maps

### Athlete Management
- Athlete profiles
- Training history
- Performance tracking
- FTP-based analysis

### Visualization
- High-performance Qt Charts rendering
- Multiple synchronized charts
- Zoom and selection tools
- Activity overlays
- Map display and route inspection

### Cross-Platform
- macOS
- Windows
- Linux 

---

## Technical Details

### Language
- C++23

### Build System
- CMake 3.21+

### GUI Framework
- Qt 6

### Qt Modules Used
- Qt Widgets
- Qt Charts
- Qt SQL
- Qt Network

### External Dependencies

#### Garmin FIT SDK
The application automatically downloads the Garmin FIT C++ SDK during configuration using CMake FetchContent when enabled.

SDK Version:
- 21.205.0

Repository:
https://github.com/garmin/fit-cpp-sdk

---

## Project Architecture

```text
FitlyzerC
├── src/
│   ├── platform/
│   │   ├── Platform_linux.cpp
│   │   ├── Platform_macos.cpp
│   │   └── Platform_windows.cpp
│   ├── charts/
│   ├── database/
│   ├── fit/
│   ├── map/
│   └── ui/
├── resources/
│   ├── icons/
│   └── fonts/
├── CMakeLists.txt
└── resources.qrc
```

The application contains platform-specific implementations for Windows, macOS, and Linux while sharing the majority of its codebase.

---

# Build Requirements

## Common Requirements

### Required Software

| Component | Version |
|-----------|----------|
| CMake | 3.21+ |
| C++ Compiler | C++23 capable |
| Qt | 6.x |
| Ninja | Recommended |
| Git | Latest |

---

# macOS Build

## Install Dependencies

Using Homebrew:

```bash
brew update

brew install cmake
brew install ninja
brew install qt
```

Add Qt to the environment:

```bash
export PATH="/opt/homebrew/opt/qt/bin:$PATH"
```

Verify installation:

```bash
cmake --version
qmake --version
```

## Configure

```bash
git clone <repository>
cd FitlyzerC

cmake \
  -S . \
  -B build/macos \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release
```

## Build

```bash
cmake --build build/macos
```

## Build + Package DMG (Automatic)

Use the preset below to build Release and generate a DMG in one command:

```bash
cmake --build --preset macos-dmg
```

In VS Code with CMake Tools, select the `macos-dmg` configure preset first, then select the `macos-dmg` build preset. The plain `macos` preset is a normal app build and does not create a DMG.

This runs the custom `dmg` target (backed by CPack DragNDrop) and produces:

```text
FitlyzerC-<version>.dmg
```

Executable:

```text
build/macos/FitlyzerC
```

---

# Windows Build

## Install Dependencies

Install:

1. Visual Studio 2022
   - Desktop development with C++
2. CMake
3. Ninja
4. Qt 6 (MSVC version)

Recommended Qt installation:

```text
C:\Qt\
```

The project automatically attempts to detect Qt installations located under:

```text
C:\Qt\<version>\msvc*\lib\cmake\Qt6
```

## Configure

Developer Command Prompt:

```cmd
cmake ^
  -S . ^
  -B build\windows ^
  -G Ninja ^
  -DCMAKE_BUILD_TYPE=Release
```

## Build

```cmd
cmake --build build\windows
```

## Build + Package NSIS Installer

Use the dedicated packaging preset to build Release and generate the NSIS installer in one command:

```cmd
cmake --build --preset windows-package
```

In VS Code with CMake Tools, select the `windows` configure preset first, then select the `windows-package` build preset. The plain `windows` build preset only builds the app binary and does not create the installer.

This runs the CPack `PACKAGE` target and produces:

```text
FitlyzerC-<version>-Setup.exe
```

### Deployment

The build system automatically runs:

```text
windeployqt
```

when available, copying required Qt DLLs and plugins next to the executable.

Executable:

```text
build\windows\FitlyzerC.exe
```

---

# Packaging (One Command)

Use the packaging driver script to configure, build, and run CPack for the current platform:

```bash
cmake -P scripts/package.cmake
```

Default preset mapping:

- macOS -> `macos`
- Windows -> `windows`
- Linux -> `linux`

You can override the preset when needed (for example MinGW on Windows):

```bash
cmake -DFITLYZER_PRESET=windows-mingw -P scripts/package.cmake
```

Expected package outputs:

- Windows: `FitlyzerC-<version>-Setup.exe`
- macOS: `FitlyzerC-<version>.dmg`

---

# Linux Build

## Common Linux Requirements

The following tools are required on all Linux distributions:

- C++ compiler with C++23 support (GCC 13+ or Clang 17+ recommended)
- CMake 3.21+
- Ninja
- Qt 6 Base
- Qt 6 Charts
- Git

---

## Arch Linux

### Install Dependencies

```bash
sudo pacman -Syu

sudo pacman -S \
    base-devel \
    cmake \
    ninja \
    qt6-base \
    qt6-charts \
    git
```

### Configure

```bash
cmake \
  -S . \
  -B build/linux \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release
```

### Build

```bash
cmake --build build/linux
```

Executable:

```text
build/linux/FitlyzerC
```

---

## Debian 12 / Ubuntu 24.04+

### Install Dependencies

```bash
sudo apt update

sudo apt install -y \
    build-essential \
    cmake \
    ninja-build \
    qt6-base-dev \
    libqt6charts6-dev \
    git
```

### Configure

```bash
cmake \
  -S . \
  -B build/linux \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release
```

### Build

```bash
cmake --build build/linux
```

Executable:

```text
build/linux/FitlyzerC
```

---

## Fedora

### Install Dependencies

```bash
sudo dnf update

sudo dnf install -y \
    gcc-c++ \
    cmake \
    ninja-build \
    qt6-qtbase-devel \
    qt6-qtcharts-devel \
    git
```

### Configure

```bash
cmake \
  -S . \
  -B build/linux \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release
```

### Build

```bash
cmake --build build/linux
```

Executable:

```text
build/linux/FitlyzerC
```

---

## Running

After building:

```bash
./build/linux/FitlyzerC
```

or from the build directory:

```bash
cd build/linux
./FitlyzerC
```

---

# Garmin FIT SDK

The build system supports two modes.

## Automatic Download (Default)

No action required.

During configuration, CMake downloads:

```text
https://github.com/garmin/fit-cpp-sdk
```

and builds the SDK automatically.

## Manual SDK Location

```bash
cmake \
  -S . \
  -B build \
  -DGARMIN_FIT_SDK_DIR=/path/to/fit-sdk
```

The path may point to:

```text
fit-sdk/
fit-sdk/src/
fit-sdk/cpp-sdk/src/
```

---

# Running the Application

After building:

```bash
./FitlyzerC
```

or on Windows:

```cmd
FitlyzerC.exe
```

Typical workflow:

1. Start FitlyzerC.
2. Create or select an athlete.
3. Import FIT files.
4. Inspect charts and maps.
5. Analyze intervals and training sessions.
6. Review performance metrics.

---

# Release Builds

## macOS

Typical release workflow:

```bash
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
```

Package using:

```bash
macdeployqt
```

Optionally create:

```text
FitlyzerC.app
FitlyzerC.dmg
```

## Windows

```cmd
cmake --build build\windows --config Release
```

Deploy using:

```cmd
windeployqt
```

Create installer using:
- NSIS
- Inno Setup
- WiX

## Linux

Package as:
- AppImage
- Flatpak
- Native packages

---

# Troubleshooting

## Qt Not Found

Specify Qt explicitly:

```bash
cmake \
  -DCMAKE_PREFIX_PATH=/path/to/Qt \
  -S . \
  -B build
```

## Garmin SDK Errors

Ensure either:

```text
FITLYZERC_AUTO_DOWNLOAD_FITSDK=ON
```

or:

```text
GARMIN_FIT_SDK_DIR=<valid path>
```

is configured.

## Clean Build

```bash
rm -rf build
```

Then reconfigure and rebuild.

---

# License

Do whatever you want with it, just point to my repository and attibute. Except dopers, they can't use it. 
