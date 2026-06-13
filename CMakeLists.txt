cmake_minimum_required(VERSION 3.21)

project(
    FitlyzerC
    VERSION 0.1.0
    LANGUAGES CXX
)

set(FITLYZER_VERSION_MAJOR 0)
set(FITLYZER_VERSION_MINOR 1)
set(FITLYZER_VERSION_PATCH 0)

set(FITLYZER_VERSION
    "${FITLYZER_VERSION_MAJOR}.${FITLYZER_VERSION_MINOR}.${FITLYZER_VERSION_PATCH}")

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
    MACOSX_BUNDLE
    ${FITLYZERC_SOURCES}
    resources.qrc
)

set_target_properties(FitlyzerC PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "FitlyzerC"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.fitlyzer.fitlyzerc"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
)

if(WIN32)
    message(STATUS "Building FitlyzerC for Windows")
    target_sources(FitlyzerC PRIVATE
        "${CMAKE_SOURCE_DIR}/resources/windows/FitlyzerC.rc"
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

if(APPLE)
    set(MACOS_ICON
        "${CMAKE_SOURCE_DIR}/resources/icons/appicon.icns")

    target_sources(FitlyzerC PRIVATE
        ${MACOS_ICON}
    )

    set_source_files_properties(
        ${MACOS_ICON}
        PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )

    set_target_properties(
        FitlyzerC
        PROPERTIES
        MACOSX_BUNDLE_ICON_FILE appicon.icns
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

if(APPLE)
    # Deploy Qt frameworks/plugins into the .app bundle after build.
    get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
    find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${QT_BIN_DIR}")

    # Help macdeployqt resolve Homebrew-provided transitive dylibs used by Qt plugins.
    set(MACDEPLOYQT_EXTRA_ARGS)
    foreach(MACDEPLOYQT_LIBPATH IN ITEMS
        "/opt/homebrew/lib"
        "/opt/homebrew/opt/brotli/lib"
        "/opt/homebrew/opt/webp/lib"
    )
        if(EXISTS "${MACDEPLOYQT_LIBPATH}")
            list(APPEND MACDEPLOYQT_EXTRA_ARGS "-libpath=${MACDEPLOYQT_LIBPATH}")
        endif()
    endforeach()

    if(MACDEPLOYQT_EXECUTABLE)
        add_custom_command(
            TARGET FitlyzerC
            POST_BUILD
            COMMAND
                /bin/chmod
                -R
                u+w
                "$<TARGET_BUNDLE_DIR:FitlyzerC>"
            COMMAND
                "${MACDEPLOYQT_EXECUTABLE}"
                "$<TARGET_BUNDLE_DIR:FitlyzerC>"
                ${MACDEPLOYQT_EXTRA_ARGS}
                -always-overwrite
                -no-strip
                -codesign=-
            COMMENT
                "Deploying Qt runtime with macdeployqt"
            VERBATIM
        )
    else()
        message(WARNING "macdeployqt not found. Qt frameworks will not be embedded automatically.")
    endif()
endif()

include(GNUInstallDirs)

install(
    TARGETS FitlyzerC
    BUNDLE DESTINATION .
    RUNTIME DESTINATION .
)

if(WIN32)
    # Ensure CPack installer contains all Qt/MSVC runtime dependencies.
    if(COMMAND qt_generate_deploy_app_script)
        qt_generate_deploy_app_script(
            TARGET FitlyzerC
            OUTPUT_SCRIPT FITLYZERC_DEPLOY_SCRIPT
            NO_UNSUPPORTED_PLATFORM_ERROR
            DEPLOY_TOOL_OPTIONS --no-translations
        )
        install(SCRIPT ${FITLYZERC_DEPLOY_SCRIPT})
    else()
        message(WARNING "qt_generate_deploy_app_script is unavailable; installer may miss runtime dependencies.")
    endif()
    
    # Also run windeployqt during install to ensure all Qt DLLs are deployed
    get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
    
    install(CODE
        "execute_process(
            COMMAND \"${QT_BIN_DIR}/windeployqt.exe\"
                --no-translations
                \"\${CMAKE_INSTALL_PREFIX}/FitlyzerC.exe\"
            RESULT_VARIABLE QT_DEPLOY_RESULT
        )
        if(NOT QT_DEPLOY_RESULT EQUAL 0)
            message(WARNING \"windeployqt failed with code \${QT_DEPLOY_RESULT}\")
        endif()
        "
    )
endif()

set(CPACK_PACKAGE_NAME "FitlyzerC")
set(CPACK_PACKAGE_VENDOR "Fitlyzer")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "FitlyzerC-${PROJECT_VERSION}")

if(WIN32)
    set(CPACK_GENERATOR NSIS)
    set(CPACK_PACKAGE_FILE_NAME "FitlyzerC-${PROJECT_VERSION}-Setup")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/resources/icons/appicon.ico")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_CREATE_ICONS_EXTRA
"CreateShortCut '$DESKTOP\\\\FitlyzerC.lnk' '$INSTDIR\\\\FitlyzerC.exe'
CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\FitlyzerC.lnk' '$INSTDIR\\\\FitlyzerC.exe'")
elseif(APPLE)
    set(CPACK_GENERATOR DragNDrop)
    set(CPACK_PACKAGE_FILE_NAME "FitlyzerC-${PROJECT_VERSION}")
else()
    set(CPACK_PACKAGE_FILE_NAME "FitlyzerC-${PROJECT_VERSION}")
endif()

include(CPack)

if(APPLE)
    # Convenience target so macOS packaging is explicit and easy to invoke.
    add_custom_target(dmg DEPENDS package)
endif()