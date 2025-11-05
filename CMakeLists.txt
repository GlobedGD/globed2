cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()

project(globed2 VERSION 1.0.0)

# Include geode early, since we need the platform define

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# Options

option(GLOBED_VOICE_SUPPORT "Enable voice chat support" ON)
option(GLOBED_QUIC_SUPPORT "Enable QUIC support" ON)
option(GLOBED_ADVANCED_DNS_SUPPORT "Enable advanced DNS support" ON)
option(GLOBED_DEBUG "Enable debug mode" OFF)
option(GLOBED_RELEASE "Enable release mode optimizations" OFF)
option(GLOBED_OSS_BUILD "Open source build that does not require closed-source dependencies" OFF)
set(GLOBED_MODULES "" CACHE STRING "Enabled modules (comma-separated)")
set(GLOBED_DEFAULT_MAIN_SERVER_URL "qunet://globed.dev" CACHE STRING "Default main server URL")

set(QUNET_VERSION "68dd378")
set(SERVER_SHARED_VERSION "f9670e3")
set(REQUIRED_GEODE_VERSION "4bb9f16")

# Run the pre-build python script

find_package(Python3 REQUIRED)
execute_process(
    COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/pre-build.py"
        --build-dir "${CMAKE_CURRENT_BINARY_DIR}"
        --output-file "${CMAKE_CURRENT_BINARY_DIR}/prebuild-gen.cmake"
        --modules "${GLOBED_MODULES}"
        --default-server-url "${GLOBED_DEFAULT_MAIN_SERVER_URL}"
        --platform "${GEODE_TARGET_PLATFORM}"
        --debug "${GLOBED_DEBUG}"
        --release "${GLOBED_RELEASE}"
        --oss-build "${GLOBED_OSS_BUILD}"
        --geode-sdk-path "$ENV{GEODE_SDK}"
        --voice-support "${GLOBED_VOICE_SUPPORT}"
        --quic "${GLOBED_QUIC_SUPPORT}"
        --advanced-dns "${GLOBED_ADVANCED_DNS_SUPPORT}"
        --server-shared-ver "${SERVER_SHARED_VERSION}"
        --qunet-ver "${QUNET_VERSION}"
        --compiler-id "${CMAKE_CXX_COMPILER_ID}"
        --compiler-frontend "${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}"
        --compiler-version "${CMAKE_CXX_COMPILER_VERSION}"
        --require-geode "${REQUIRED_GEODE_VERSION}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE PREBUILD_RESULT
)

if (NOT PREBUILD_RESULT EQUAL 0)
    message(FATAL_ERROR "Pre-build script failed with exit code ${PREBUILD_RESULT}, see above for details")
endif()

include(${CMAKE_CURRENT_BINARY_DIR}/prebuild-gen.cmake)

setup_geode_mod(${PROJECT_NAME} LINK_TYPE PRIVATE)

# trick to reconfigure when the script changes
configure_file(pre-build.py ${CMAKE_CURRENT_BINARY_DIR}/pre-build-dummy.py COPYONLY)
