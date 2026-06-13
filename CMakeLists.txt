cmake_minimum_required(VERSION 3.21)

project(FitlyzerC)

include(FetchContent)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32 AND NOT Qt6_DIR AND NOT CMAKE_PREFIX_PATH)
    # Try common Qt installation locations for MSVC builds.
    file(GLOB QT6_MSVC_CANDIDATES "C:/Qt/*/msvc*/lib/cmake/Qt6")
    list(SORT QT6_MSVC_CANDIDATES COMPARE NATURAL ORDER DESCENDING)

    foreach(QT6_CANDIDATE IN LISTS QT6_MSVC_CANDIDATES)
        if(EXISTS "${QT6_CANDIDATE}/Qt6Config.cmake")
            set(Qt6_DIR "${QT6_CANDIDATE}" CACHE PATH "Path to Qt6Config.cmake" FORCE)
            get_filename_component(QT6_PREFIX "${QT6_CANDIDATE}/../../.." ABSOLUTE)
            set(CMAKE_PREFIX_PATH "${QT6_PREFIX}" CACHE STRING "Qt prefix path" FORCE)
            message(STATUS "Auto-detected Qt6 for Windows preset at: ${Qt6_DIR}")
            break()
        endif()
    endforeach()
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Network Charts)

qt_standard_project_setup()

#
# Garmin FIT SDK
#

set(
    GARMIN_FIT_SDK_DIR
    ""
    CACHE PATH
    "Path to Garmin FIT SDK source directory (root, src, or cpp-sdk/src)"
)

option(
    FITLYZERC_AUTO_DOWNLOAD_FITSDK
    "Automatically download Garmin FIT SDK"
    ON
)

set(
    GARMIN_FIT_SDK_VERSION
    "21.205.0"
    CACHE STRING
    "Pinned Garmin FIT C++ SDK version tag"
)

if(NOT GARMIN_FIT_SDK_DIR)
    if(NOT FITLYZERC_AUTO_DOWNLOAD_FITSDK)
        message(FATAL_ERROR
            "Garmin FIT SDK not configured. Set GARMIN_FIT_SDK_DIR or enable FITLYZERC_AUTO_DOWNLOAD_FITSDK."
        )
    endif()

    message(STATUS "Downloading Garmin FIT SDK")

    FetchContent_Declare(
        GarminFitSDK
        URL "https://github.com/garmin/fit-cpp-sdk/archive/refs/tags/${GARMIN_FIT_SDK_VERSION}.zip"
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    FetchContent_MakeAvailable(GarminFitSDK)

    set(
        GARMIN_FIT_SDK_DIR
        "${garminfitsdk_SOURCE_DIR}"
    )
endif()

if(NOT IS_ABSOLUTE "${GARMIN_FIT_SDK_DIR}")
    get_filename_component(GARMIN_FIT_SDK_DIR "${GARMIN_FIT_SDK_DIR}" ABSOLUTE BASE_DIR "${CMAKE_SOURCE_DIR}")
endif()

if(EXISTS "${GARMIN_FIT_SDK_DIR}/fit_decode.cpp")
    # GARMIN_FIT_SDK_DIR already points at the source directory.
elseif(EXISTS "${GARMIN_FIT_SDK_DIR}/src/fit_decode.cpp")
    set(GARMIN_FIT_SDK_DIR "${GARMIN_FIT_SDK_DIR}/src")
elseif(EXISTS "${GARMIN_FIT_SDK_DIR}/cpp-sdk/src/fit_decode.cpp")
    set(GARMIN_FIT_SDK_DIR "${GARMIN_FIT_SDK_DIR}/cpp-sdk/src")
endif()

if(NOT EXISTS "${GARMIN_FIT_SDK_DIR}/fit_decode.cpp")
    message(FATAL_ERROR
        "Garmin FIT SDK not found: ${GARMIN_FIT_SDK_DIR}"
    )
endif()

file(GLOB FIT_SDK_SOURCES
    "${GARMIN_FIT_SDK_DIR}/*.cpp"
)

add_library(FitSDK STATIC
    ${FIT_SDK_SOURCES}
)

target_include_directories(FitSDK PUBLIC
    "${GARMIN_FIT_SDK_DIR}"
)

#
# Main application
#

file(GLOB_RECURSE FITLYZERC_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.h"
)

set(FITLYZERC_PLATFORM_SOURCES
    "${CMAKE_SOURCE_DIR}/src/platform/Platform.cpp"
    "${CMAKE_SOURCE_DIR}/src/platform/Platform_linux.cpp"
    "${CMAKE_SOURCE_DIR}/src/platform/Platform_macos.cpp"
    "${CMAKE_SOURCE_DIR}/src/platform/Platform_windows.cpp"
)

list(REMOVE_ITEM FITLYZERC_SOURCES ${FITLYZERC_PLATFORM_SOURCES})

qt_add_executable(FitlyzerC
    ${FITLYZERC_SOURCES}
    resources.qrc
)

if(WIN32)
    message(STATUS "Building FitlyzerC for Windows")
    target_sources(FitlyzerC PRIVATE
        "${CMAKE_SOURCE_DIR}/src/platform/Platform.cpp"
        "${CMAKE_SOURCE_DIR}/src/platform/Platform_windows.cpp"
    )
elseif(APPLE)
    message(STATUS "Building FitlyzerC for macOS")
    target_sources(FitlyzerC PRIVATE
        "${CMAKE_SOURCE_DIR}/src/platform/Platform.cpp"
        "${CMAKE_SOURCE_DIR}/src/platform/Platform_macos.cpp"
    )
elseif(UNIX)
    message(STATUS "Building FitlyzerC for Linux")
    target_sources(FitlyzerC PRIVATE
        "${CMAKE_SOURCE_DIR}/src/platform/Platform.cpp"
        "${CMAKE_SOURCE_DIR}/src/platform/Platform_linux.cpp"
    )
endif()

target_include_directories(FitlyzerC PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(FitlyzerC PRIVATE
    Qt6::Widgets
    Qt6::Sql
    Qt6::Network
    Qt6::Charts
    FitSDK
)

if(WIN32)
    # Deploy Qt runtime DLLs/plugins next to the built executable.
    get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_BIN_DIR}")

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET FitlyzerC POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                "$<$<CONFIG:Debug>:--debug>$<$<NOT:$<CONFIG:Debug>>:--release>"
                --no-translations
                "$<TARGET_FILE:FitlyzerC>"
            COMMENT "Deploying Qt runtime with windeployqt"
            VERBATIM
        )
    else()
        message(WARNING "windeployqt not found. Runtime Qt DLLs will not be copied automatically.")
    endif()
endif()