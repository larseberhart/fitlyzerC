cmake_minimum_required(VERSION 3.21)

# Usage:
#   cmake -P scripts/package.cmake
#   cmake -DFITLYZER_PRESET=windows-mingw -P scripts/package.cmake

get_filename_component(FITLYZER_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

if(NOT DEFINED FITLYZER_PRESET OR FITLYZER_PRESET STREQUAL "")
    if(WIN32)
        set(FITLYZER_PRESET "windows")
    elseif(APPLE)
        set(FITLYZER_PRESET "macos")
    else()
        set(FITLYZER_PRESET "linux-appimage")
    endif()
endif()

set(FITLYZER_BUILD_DIR "${FITLYZER_SOURCE_DIR}/build/${FITLYZER_PRESET}")
set(FITLYZER_CPACK_CONFIG "${FITLYZER_BUILD_DIR}/CPackConfig.cmake")

find_program(CPACK_EXECUTABLE cpack)
if(NOT CPACK_EXECUTABLE)
    message(FATAL_ERROR "cpack executable was not found in PATH.")
endif()

message(STATUS "Packaging FitlyzerC with preset: ${FITLYZER_PRESET}")
message(STATUS "Source dir: ${FITLYZER_SOURCE_DIR}")
message(STATUS "Build dir: ${FITLYZER_BUILD_DIR}")

# Configure for Release by default so package outputs use release binaries.
execute_process(
    COMMAND "${CMAKE_COMMAND}" --preset "${FITLYZER_PRESET}"
    WORKING_DIRECTORY "${FITLYZER_SOURCE_DIR}"
    RESULT_VARIABLE FITLYZER_CONFIGURE_RESULT
)
if(NOT FITLYZER_CONFIGURE_RESULT EQUAL 0)
    message(FATAL_ERROR "Configure failed with exit code ${FITLYZER_CONFIGURE_RESULT}.")
endif()

set(FITLYZER_BUILD_COMMAND "${CMAKE_COMMAND}" --build "${FITLYZER_BUILD_DIR}")
if(WIN32 AND FITLYZER_PRESET STREQUAL "windows")
    list(APPEND FITLYZER_BUILD_COMMAND --config Release)
endif()

execute_process(
    COMMAND ${FITLYZER_BUILD_COMMAND}
    WORKING_DIRECTORY "${FITLYZER_SOURCE_DIR}"
    RESULT_VARIABLE FITLYZER_BUILD_RESULT
)
if(NOT FITLYZER_BUILD_RESULT EQUAL 0)
    message(FATAL_ERROR "Build failed with exit code ${FITLYZER_BUILD_RESULT}.")
endif()

if(NOT EXISTS "${FITLYZER_CPACK_CONFIG}")
    message(FATAL_ERROR "Missing CPack config: ${FITLYZER_CPACK_CONFIG}")
endif()

set(FITLYZER_CPACK_COMMAND
    "${CPACK_EXECUTABLE}"
    --config "${FITLYZER_CPACK_CONFIG}"
)

if(WIN32 AND FITLYZER_PRESET STREQUAL "windows")
    list(APPEND FITLYZER_CPACK_COMMAND -C Release)
endif()

execute_process(
    COMMAND ${FITLYZER_CPACK_COMMAND}
    WORKING_DIRECTORY "${FITLYZER_SOURCE_DIR}"
    RESULT_VARIABLE FITLYZER_CPACK_RESULT
)
if(NOT FITLYZER_CPACK_RESULT EQUAL 0)
    message(FATAL_ERROR "CPack failed with exit code ${FITLYZER_CPACK_RESULT}.")
endif()

message(STATUS "Packaging completed successfully.")
message(STATUS "Artifacts are available in: ${FITLYZER_SOURCE_DIR}")
