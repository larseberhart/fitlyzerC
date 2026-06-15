cmake_minimum_required(VERSION 3.21)

project(
    FitlyzerC
    VERSION 0.5.0
    LANGUAGES CXX
)

include(FetchContent)
include(ExternalProject)
include(CTest)
include(CheckIPOSupported)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(
    FITLYZERC_ENABLE_RELEASE_IPO
    "Enable interprocedural optimization (LTO) for Release and RelWithDebInfo"
    ON
)

# Build profile tuning:
# - Debug: prioritize fast compile + clear debugging
# - Release/RelWithDebInfo: prioritize runtime execution speed
if(MSVC)
    add_compile_options(
        "$<$<CONFIG:Debug>:/Od>"
        "$<$<CONFIG:Debug>:/Zi>"
        "$<$<CONFIG:Release>:/O2>"
        "$<$<CONFIG:RelWithDebInfo>:/O2>"
    )
else()
    add_compile_options(
        "$<$<CONFIG:Debug>:-O0>"
        "$<$<CONFIG:Debug>:-g>"
        "$<$<CONFIG:Release>:-O3>"
        "$<$<CONFIG:RelWithDebInfo>:-O2>"
        "$<$<CONFIG:RelWithDebInfo>:-g>"
    )
endif()

if(FITLYZERC_ENABLE_RELEASE_IPO)
    check_ipo_supported(RESULT FITLYZERC_IPO_SUPPORTED OUTPUT FITLYZERC_IPO_ERROR)
    if(FITLYZERC_IPO_SUPPORTED)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
    else()
        message(WARNING "Release IPO/LTO is not supported by this toolchain: ${FITLYZERC_IPO_ERROR}")
    endif()
endif()


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

option(
    FITLYZERC_AUTO_DOWNLOAD_FFMPEG
    "Automatically download FFmpeg source when a local tree is unavailable"
    ON
)

set(
    FITLYZER_FFMPEG_PATH
    ""
    CACHE STRING
    "Path to bundled FFmpeg executable (absolute or relative to app directory)"
)

option(
    FITLYZERC_BUNDLE_FFMPEG
    "Bundle FFmpeg with FitlyzerC releases"
    ON
)

set(
    FITLYZERC_FFMPEG_SOURCE_DIR
    "${CMAKE_SOURCE_DIR}/third_party/ffmpeg"
    CACHE PATH
    "Path to FFmpeg source tree (expects a configure script; auto-download is used if absent)"
)

set(
    FITLYZERC_FFMPEG_GIT_REPOSITORY
    "https://git.ffmpeg.org/ffmpeg.git"
    CACHE STRING
    "Git repository used to fetch FFmpeg source when auto-download is enabled"
)

set(
    FITLYZERC_FFMPEG_GIT_TAG
    "master"
    CACHE STRING
    "Git branch, tag, or commit used when auto-downloading FFmpeg source"
)

set(
    FITLYZERC_FFMPEG_BINARY_PATH
    ""
    CACHE FILEPATH
    "Path to a prebuilt FFmpeg executable to bundle if source build is unavailable"
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

if(APPLE)
    find_program(CODESIGN_EXECUTABLE codesign HINTS "/usr/bin")
endif()

set(FITLYZERC_BUNDLED_FFMPEG_BINARY "")
set(FITLYZERC_RUNTIME_FFMPEG_PATH "")

if(FITLYZERC_BUNDLE_FFMPEG)
    set(_ffmpeg_suffix "")
    if(WIN32)
        set(_ffmpeg_suffix ".exe")
    endif()

    set(_ffmpeg_prebuilt "${FITLYZERC_FFMPEG_BINARY_PATH}")
    if(NOT _ffmpeg_prebuilt)
        if(WIN32)
            set(_ffmpeg_prebuilt "${CMAKE_SOURCE_DIR}/resources/ffmpeg/windows/ffmpeg.exe")
        elseif(APPLE)
            set(_ffmpeg_prebuilt "${CMAKE_SOURCE_DIR}/resources/ffmpeg/macos/ffmpeg")
        else()
            set(_ffmpeg_prebuilt "${CMAKE_SOURCE_DIR}/resources/ffmpeg/linux/ffmpeg")
        endif()
    endif()

    if(_ffmpeg_prebuilt AND NOT IS_ABSOLUTE "${_ffmpeg_prebuilt}")
        get_filename_component(_ffmpeg_prebuilt "${_ffmpeg_prebuilt}" ABSOLUTE BASE_DIR "${CMAKE_SOURCE_DIR}")
    endif()

    if(APPLE AND EXISTS "${_ffmpeg_prebuilt}")
        set(FITLYZERC_BUNDLED_FFMPEG_BINARY "${_ffmpeg_prebuilt}")
        message(STATUS "Using bundled macOS FFmpeg resource: ${FITLYZERC_BUNDLED_FFMPEG_BINARY}")
    endif()

    if(NOT FITLYZERC_BUNDLED_FFMPEG_BINARY)
        set(_ffmpeg_source_dir "${FITLYZERC_FFMPEG_SOURCE_DIR}")
        if(_ffmpeg_source_dir AND NOT IS_ABSOLUTE "${_ffmpeg_source_dir}")
            get_filename_component(_ffmpeg_source_dir "${_ffmpeg_source_dir}" ABSOLUTE BASE_DIR "${CMAKE_SOURCE_DIR}")
        endif()

        set(_ffmpeg_download_args)
        if(_ffmpeg_source_dir AND EXISTS "${_ffmpeg_source_dir}/configure")
            message(STATUS "FFmpeg source tree found: ${_ffmpeg_source_dir}")
        elseif(FITLYZERC_AUTO_DOWNLOAD_FFMPEG)
            set(_ffmpeg_source_dir "${CMAKE_BINARY_DIR}/_deps/ffmpeg-src")
            set(_ffmpeg_download_args
                GIT_REPOSITORY "${FITLYZERC_FFMPEG_GIT_REPOSITORY}"
                GIT_TAG "${FITLYZERC_FFMPEG_GIT_TAG}"
                GIT_SHALLOW TRUE
            )
            message(STATUS
                "FFmpeg source tree not found; cloning ${FITLYZERC_FFMPEG_GIT_REPOSITORY} (${FITLYZERC_FFMPEG_GIT_TAG})"
            )
        endif()

        if(_ffmpeg_source_dir)
            find_program(FITLYZERC_BASH_EXECUTABLE NAMES bash)
            find_program(FITLYZERC_MAKE_EXECUTABLE NAMES make gmake mingw32-make)

            if(FITLYZERC_BASH_EXECUTABLE AND FITLYZERC_MAKE_EXECUTABLE)
                set(FITLYZERC_FFMPEG_INSTALL_DIR "${CMAKE_BINARY_DIR}/ffmpeg/install")
                set(FITLYZERC_FFMPEG_BUILD_DIR "${CMAKE_BINARY_DIR}/ffmpeg/build")

                set(_ffmpeg_configure
                    "${FITLYZERC_BASH_EXECUTABLE}" "${_ffmpeg_source_dir}/configure"
                    "--prefix=${FITLYZERC_FFMPEG_INSTALL_DIR}"
                    "--disable-shared"
                    "--enable-static"
                    "--disable-debug"
                    "--disable-doc"
                    "--enable-ffmpeg"
                    "--disable-ffplay"
                    "--disable-ffprobe"
                )

                ExternalProject_Add(FitlyzerFFmpeg
                    SOURCE_DIR "${_ffmpeg_source_dir}"
                    BINARY_DIR "${FITLYZERC_FFMPEG_BUILD_DIR}"
                    ${_ffmpeg_download_args}
                    CONFIGURE_COMMAND ${_ffmpeg_configure}
                    BUILD_COMMAND "${FITLYZERC_MAKE_EXECUTABLE}" -j
                    INSTALL_COMMAND "${FITLYZERC_MAKE_EXECUTABLE}" install
                    BUILD_BYPRODUCTS "${FITLYZERC_FFMPEG_INSTALL_DIR}/bin/ffmpeg${_ffmpeg_suffix}"
                    LOG_DOWNLOAD ON
                    LOG_CONFIGURE ON
                    LOG_BUILD ON
                    LOG_INSTALL ON
                )

                add_dependencies(FitlyzerC FitlyzerFFmpeg)
                set(FITLYZERC_BUNDLED_FFMPEG_BINARY "${FITLYZERC_FFMPEG_INSTALL_DIR}/bin/ffmpeg${_ffmpeg_suffix}")
                if(_ffmpeg_download_args)
                    message(STATUS "FFmpeg will be downloaded and built from source into ${_ffmpeg_source_dir}")
                else()
                    message(STATUS "FFmpeg will be built from source: ${_ffmpeg_source_dir}")
                endif()
            else()
                message(WARNING "FFmpeg source was found but build tools are missing (bash/make). Falling back to prebuilt FFmpeg lookup.")
            endif()
        endif()
    endif()

    if(NOT FITLYZERC_BUNDLED_FFMPEG_BINARY)
        if(EXISTS "${_ffmpeg_prebuilt}")
            set(FITLYZERC_BUNDLED_FFMPEG_BINARY "${_ffmpeg_prebuilt}")
            message(STATUS "Using prebuilt bundled FFmpeg: ${FITLYZERC_BUNDLED_FFMPEG_BINARY}")
        else()
            message(WARNING "No bundled FFmpeg source or binary found. FitlyzerC will fall back to system FFmpeg.")
        endif()
    endif()

    if(FITLYZERC_BUNDLED_FFMPEG_BINARY)
        if(WIN32)
            set(FITLYZERC_RUNTIME_FFMPEG_PATH "ffmpeg.exe")
            set(FITLYZERC_RUNTIME_FFMPEG_DEST "$<TARGET_FILE_DIR:FitlyzerC>/ffmpeg.exe")
            set(FITLYZERC_INSTALL_FFMPEG_DEST ".")
            get_filename_component(FITLYZERC_FFMPEG_RUNTIME_DIR "${FITLYZERC_BUNDLED_FFMPEG_BINARY}" DIRECTORY)
            file(GLOB FITLYZERC_FFMPEG_RUNTIME_DLLS CONFIGURE_DEPENDS
                "${FITLYZERC_FFMPEG_RUNTIME_DIR}/*.dll"
            )
        elseif(APPLE)
            set(FITLYZERC_RUNTIME_FFMPEG_PATH "ffmpeg")
            set(FITLYZERC_RUNTIME_FFMPEG_DEST "$<TARGET_BUNDLE_DIR:FitlyzerC>/Contents/MacOS/ffmpeg")
            set(FITLYZERC_INSTALL_FFMPEG_DEST "FitlyzerC.app/Contents/MacOS")
        else()
            set(FITLYZERC_RUNTIME_FFMPEG_PATH "ffmpeg")
            set(FITLYZERC_RUNTIME_FFMPEG_DEST "$<TARGET_FILE_DIR:FitlyzerC>/ffmpeg")
            set(FITLYZERC_INSTALL_FFMPEG_DEST ".")
        endif()

        add_custom_command(TARGET FitlyzerC POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${FITLYZERC_BUNDLED_FFMPEG_BINARY}"
                "${FITLYZERC_RUNTIME_FFMPEG_DEST}"
            COMMENT "Copying bundled FFmpeg next to FitlyzerC"
            VERBATIM
        )

        if(WIN32 AND FITLYZERC_FFMPEG_RUNTIME_DLLS)
            add_custom_command(TARGET FitlyzerC POST_BUILD
                COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                    ${FITLYZERC_FFMPEG_RUNTIME_DLLS}
                    "$<TARGET_FILE_DIR:FitlyzerC>"
                COMMENT "Copying bundled FFmpeg runtime DLLs next to FitlyzerC"
                VERBATIM
            )
        endif()

        if(NOT WIN32)
            add_custom_command(TARGET FitlyzerC POST_BUILD
                COMMAND /bin/chmod +x "${FITLYZERC_RUNTIME_FFMPEG_DEST}"
                COMMENT "Ensuring bundled FFmpeg is executable"
                VERBATIM
            )
        endif()

        if(APPLE AND CODESIGN_EXECUTABLE)
            add_custom_command(TARGET FitlyzerC POST_BUILD
                COMMAND "${CODESIGN_EXECUTABLE}"
                    --force
                    --sign
                    -
                    "${FITLYZERC_RUNTIME_FFMPEG_DEST}"
                COMMAND "${CODESIGN_EXECUTABLE}"
                    --verify
                    --strict
                    --verbose=4
                    "${FITLYZERC_RUNTIME_FFMPEG_DEST}"
                COMMENT "Re-signing and verifying bundled FFmpeg"
                VERBATIM
            )
        endif()

        install(FILES
            "${FITLYZERC_BUNDLED_FFMPEG_BINARY}"
            DESTINATION "${FITLYZERC_INSTALL_FFMPEG_DEST}"
            PERMISSIONS
                OWNER_READ OWNER_WRITE OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_READ WORLD_EXECUTE
        )

        if(WIN32 AND FITLYZERC_FFMPEG_RUNTIME_DLLS)
            install(FILES
                ${FITLYZERC_FFMPEG_RUNTIME_DLLS}
                DESTINATION "${FITLYZERC_INSTALL_FFMPEG_DEST}"
            )
        endif()
    endif()
endif()

if(FITLYZERC_RUNTIME_FFMPEG_PATH)
    target_compile_definitions(FitlyzerC PRIVATE FITLYZER_FFMPEG_PATH="${FITLYZERC_RUNTIME_FFMPEG_PATH}")
elseif(FITLYZER_FFMPEG_PATH)
    target_compile_definitions(FitlyzerC PRIVATE FITLYZER_FFMPEG_PATH="${FITLYZER_FFMPEG_PATH}")
endif()

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
    # Deploy/sign during install packaging on macOS to avoid mutating the live
    # build-tree bundle while linking.
    # Help Qt's deploy tooling resolve Homebrew-provided transitive dylibs used by Qt plugins.
    set(MACDEPLOYQT_EXTRA_ARGS)
    foreach(MACDEPLOYQT_LIBPATH IN ITEMS
        "/opt/homebrew/opt/qt/lib"
        "/opt/homebrew/lib"
        "/opt/homebrew/opt/qtdeclarative/lib"
        "/opt/homebrew/opt/qtsvg/lib"
        "/opt/homebrew/opt/qtvirtualkeyboard/lib"
        "/opt/homebrew/opt/qtshadertools/lib"
        "/opt/homebrew/opt/brotli/lib"
        "/opt/homebrew/opt/webp/lib"
    )
        if(EXISTS "${MACDEPLOYQT_LIBPATH}")
            list(APPEND MACDEPLOYQT_EXTRA_ARGS "-libpath=${MACDEPLOYQT_LIBPATH}")
        endif()
    endforeach()
endif()

include(GNUInstallDirs)

option(
    FITLYZERC_ENABLE_APPIMAGE_PACKAGE
    "Enable Linux AppImage packaging"
    OFF
)

install(
    TARGETS FitlyzerC
    BUNDLE DESTINATION .
    RUNTIME DESTINATION .
)

if(APPLE)
    if(COMMAND qt_generate_deploy_app_script)
        qt_generate_deploy_app_script(
            TARGET FitlyzerC
            OUTPUT_SCRIPT FITLYZERC_DEPLOY_SCRIPT
            NO_UNSUPPORTED_PLATFORM_ERROR
            DEPLOY_TOOL_OPTIONS
                ${MACDEPLOYQT_EXTRA_ARGS}
                -always-overwrite
                -no-strip
        )
        install(SCRIPT ${FITLYZERC_DEPLOY_SCRIPT})
    else()
        message(WARNING "qt_generate_deploy_app_script is unavailable; macOS bundle may miss runtime dependencies.")
    endif()
endif()

if(APPLE AND CODESIGN_EXECUTABLE)
    install(CODE
        "execute_process(
            COMMAND /bin/chmod -R u+w \"\${CMAKE_INSTALL_PREFIX}/FitlyzerC.app\"
            RESULT_VARIABLE FITLYZERC_STAGE_CHMOD_RESULT
        )
        if(NOT FITLYZERC_STAGE_CHMOD_RESULT EQUAL 0)
            message(FATAL_ERROR \"Failed to make staged app writable before codesign: \${FITLYZERC_STAGE_CHMOD_RESULT}\")
        endif()

        if(EXISTS \"\${CMAKE_INSTALL_PREFIX}/FitlyzerC.app/Contents/MacOS/ffmpeg\")
            execute_process(
                COMMAND \"${CODESIGN_EXECUTABLE}\"
                    --force
                    --sign
                    -
                    \"\${CMAKE_INSTALL_PREFIX}/FitlyzerC.app/Contents/MacOS/ffmpeg\"
                RESULT_VARIABLE FITLYZERC_STAGE_FFMPEG_CODESIGN_RESULT
            )
            if(NOT FITLYZERC_STAGE_FFMPEG_CODESIGN_RESULT EQUAL 0)
                message(FATAL_ERROR \"Failed to sign bundled FFmpeg in staged app: \${FITLYZERC_STAGE_FFMPEG_CODESIGN_RESULT}\")
            endif()
        endif()

        execute_process(
            COMMAND \"${CODESIGN_EXECUTABLE}\"
                --force
                --deep
                --sign
                -
                \"\${CMAKE_INSTALL_PREFIX}/FitlyzerC.app\"
            RESULT_VARIABLE FITLYZERC_STAGE_CODESIGN_RESULT
        )
        if(NOT FITLYZERC_STAGE_CODESIGN_RESULT EQUAL 0)
            message(FATAL_ERROR \"Failed to re-sign staged macOS bundle: \${FITLYZERC_STAGE_CODESIGN_RESULT}\")
        endif()

        execute_process(
            COMMAND \"${CODESIGN_EXECUTABLE}\"
                --verify
                --deep
                --strict
                --verbose=4
                \"\${CMAKE_INSTALL_PREFIX}/FitlyzerC.app\"
            RESULT_VARIABLE FITLYZERC_STAGE_VERIFY_RESULT
        )
        if(NOT FITLYZERC_STAGE_VERIFY_RESULT EQUAL 0)
            message(FATAL_ERROR \"Staged macOS bundle failed codesign verification: \${FITLYZERC_STAGE_VERIFY_RESULT}\")
        endif()
        "
    )
endif()

if(UNIX AND NOT APPLE AND FITLYZERC_ENABLE_APPIMAGE_PACKAGE)
    if(COMMAND qt_generate_deploy_app_script)
        qt_generate_deploy_app_script(
            TARGET FitlyzerC
            OUTPUT_SCRIPT FITLYZERC_DEPLOY_SCRIPT
            NO_UNSUPPORTED_PLATFORM_ERROR
            DEPLOY_TOOL_OPTIONS --no-translations
        )
        install(SCRIPT ${FITLYZERC_DEPLOY_SCRIPT})
    else()
        message(WARNING "qt_generate_deploy_app_script is unavailable; AppImage may miss runtime dependencies.")
    endif()

    install(FILES
        "${CMAKE_SOURCE_DIR}/resources/linux/FitlyzerC.desktop"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
    )

    install(FILES
        "${CMAKE_SOURCE_DIR}/resources/appicon.png"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps"
        RENAME "appicon"
    )

    install(FILES
        "${CMAKE_SOURCE_DIR}/resources/appicon.png"
        DESTINATION "."
        RENAME "appicon.png"
    )
endif()

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
set(CPACK_PACKAGE_FILE_NAME "FitlyzerC")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "FitlyzerC")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Import FIT rides, analyze training data, and export ride videos.")

# Keep packaging self-contained: do not depend on repository README/LICENSE files.
set(FITLYZERC_CPACK_LICENSE_FILE "${CMAKE_BINARY_DIR}/CPackLicense.txt")
file(WRITE "${FITLYZERC_CPACK_LICENSE_FILE}" "Do whatever you want with this software!\nExcept dopers, they can't use this...")
set(CPACK_RESOURCE_FILE_LICENSE "${FITLYZERC_CPACK_LICENSE_FILE}")

if(WIN32)
    set(CPACK_GENERATOR NSIS)
    set(CPACK_PACKAGE_FILE_NAME "FitlyzerC-Setup")
    set(CPACK_NSIS_DISPLAY_NAME "FitlyzerC")
    set(CPACK_NSIS_HELP_LINK "https://github.com/larseberhart/fitlyzerC")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/larseberhart/fitlyzerC")
    set(CPACK_NSIS_CONTACT "email@larseberhart.com")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/resources/icons/appicon.ico")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_CREATE_ICONS_EXTRA
"CreateShortCut '$DESKTOP\\\\FitlyzerC.lnk' '$INSTDIR\\\\FitlyzerC.exe'
CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\FitlyzerC.lnk' '$INSTDIR\\\\FitlyzerC.exe'")
elseif(APPLE)
    set(CPACK_GENERATOR DragNDrop)
    set(CPACK_PACKAGE_FILE_NAME "FitlyzerC")
    set(CPACK_DMG_VOLUME_NAME "FitlyzerC")
elseif(UNIX AND FITLYZERC_ENABLE_APPIMAGE_PACKAGE)
    find_program(CPACK_APPIMAGE_TOOL_EXECUTABLE appimagetool)
    if(NOT CPACK_APPIMAGE_TOOL_EXECUTABLE)
        message(FATAL_ERROR "appimagetool was not found in PATH. Install appimagetool-bin or add appimagetool to your PATH.")
    endif()

    find_program(CPACK_APPIMAGE_PATCHELF_EXECUTABLE patchelf)
    if(NOT CPACK_APPIMAGE_PATCHELF_EXECUTABLE)
        message(FATAL_ERROR "patchelf was not found in PATH. Install patchelf to build AppImages.")
    endif()

    set(CPACK_PACKAGE_ICON "appicon")
    set(CPACK_GENERATOR AppImage)
    set(CPACK_APPIMAGE_DESKTOP_FILE "FitlyzerC.desktop")
    set(CPACK_APPIMAGE_NO_APPSTREAM ON)
    set(CPACK_APPIMAGE_TOOL_EXECUTABLE "${CPACK_APPIMAGE_TOOL_EXECUTABLE}")
    set(CPACK_APPIMAGE_PATCHELF_EXECUTABLE "${CPACK_APPIMAGE_PATCHELF_EXECUTABLE}")
    set(CPACK_PACKAGE_FILE_NAME "FitlyzerC")
endif()

include(CPack)



if(APPLE)
    # Convenience target so macOS packaging is explicit and easy to invoke.
    add_custom_target(dmg DEPENDS package)
endif()

if(UNIX AND NOT APPLE AND FITLYZERC_ENABLE_APPIMAGE_PACKAGE)
    # Convenience target so Linux packaging is explicit and easy to invoke.
    add_custom_target(appimage DEPENDS package)
endif()