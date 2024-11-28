cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(globed2 VERSION 1.0.1)

if (${CMAKE_SYSTEM_NAME} STREQUAL "iOS")
    message(STATUS "Building for iOS")
    unset(CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0")
else()
    message(STATUS "Building for macOS")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()

option(GLOBED_OSS_BUILD "Open source build that does not require closed-source dependencies" OFF)
option(GLOBED_NATIVE_ARCH "Native architecture build" OFF)

if (WIN32)
    option(GLOBED_COMPILE_LIBS "Don't use precompiled libraries and compile manually" OFF)
else()
    option(GLOBED_COMPILE_LIBS "Don't use precompiled libraries and compile manually" ON)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(GLOBED_IS_CLANG ON)
else()
    set(GLOBED_IS_CLANG OFF)
endif()

if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
    set(GLOBED_IS_CLANG_CL OFF)
else()
    set(GLOBED_IS_CLANG_CL ON)
endif()

option(ENABLE_DEBUG "Debug mode" OFF)
option(GLOBED_RELEASE "Release build" OFF)

if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR ENABLE_DEBUG)
    set(GLOBED_IS_DEBUG ON)
endif()

if (GLOBED_IS_DEBUG)
    add_compile_definitions(GLOBED_DEBUG=1 GEODE_DEBUG=1 ASP_ENABLE_DEBUG=1)
elseif (GLOBED_RELEASE)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

if (ANDROID)
    file(GLOB_RECURSE OS_SOURCES "src/platform/os/android/*.cpp" "src/platform/arch/arm/*.cpp")
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS")
    file(GLOB_RECURSE OS_SOURCES "src/platform/os/ios/*.cpp" "src/platform/arch/arm/*.cpp")
elseif (APPLE)
    file(GLOB_RECURSE OS_SOURCES "src/platform/os/mac/*.cpp" "src/platform/arch/x86/*.cpp" "src/platform/arch/arm/*.cpp")
elseif (WIN32)
    file(GLOB_RECURSE OS_SOURCES "src/platform/os/windows/*.cpp" "src/platform/arch/x86/*.cpp")
else()
    message(FATAL_ERROR "Unsupported operating system")
endif()

file(GLOB_RECURSE SOURCES
    src/audio/*.cpp
    src/crypto/*.cpp
    src/data/*.cpp
    src/globed/*.cpp
    src/game/*.cpp
    src/hooks/*.cpp
    src/managers/*.cpp
    src/net/*.cpp
    src/ui/*.cpp
    src/util/*.cpp
    src/main.cpp
    ${OS_SOURCES}
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_compile_definitions(WIN32_LEAN_AND_MEAN=1)
    if (NOT MSVC)
        set(GLOBED_WINDOWS_CLANG ON)
    endif()
endif()

if (GLOBED_RELEASE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE GLOBED_RELEASE=1)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE GLOBED_EXPORTING)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

target_include_directories(${PROJECT_NAME} PRIVATE src/)
target_include_directories(${PROJECT_NAME} PRIVATE libs/)

CPMAddPackage("gh:camila314/uibuilder#3d10e3d")
CPMAddPackage(
    NAME Boost
    VERSION 1.86.0
    URL https://github.com/boostorg/boost/releases/download/boost-1.86.0/boost-1.86.0-cmake.7z
    URL_HASH SHA256=ee6e0793b5ec7d13e7181ec05d3b1aaa23615947295080e4b9930324488e078f
    OPTIONS "BOOST_ENABLE_CMAKE ON" "BOOST_INCLUDE_LIBRARIES describe\\\;stacktrace\\\;thread"
)
CPMAddPackage("gh:dankmeme01/asp2#6720133")

if (WIN32 AND GLOBED_IS_DEBUG)
    target_compile_definitions(asp PRIVATE _HAS_ITERATOR_DEBUGGING=0)
endif()

target_compile_definitions(asp PRIVATE ASP_ENABLE_FORMAT=1)
target_compile_definitions(${PROJECT_NAME} PRIVATE ASP_ENABLE_FORMAT=1)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/globed-codegen")
target_include_directories("${PROJECT_NAME}" PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/globed-codegen")
include(cmake/baked_resources_gen.cmake)
generate_baked_resources_header("${CMAKE_CURRENT_SOURCE_DIR}/embedded-resources.json" "${CMAKE_CURRENT_BINARY_DIR}/globed-codegen/embedded_resources.hpp")

if (GLOBED_NATIVE_ARCH)
    add_compile_options("-march=native")
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE "-Wno-deprecated-declarations")
endif()

target_link_libraries(${PROJECT_NAME} UIBuilder Boost::describe Boost::thread asp)

if (GLOBED_IS_DEBUG)
    target_link_libraries(${PROJECT_NAME} Boost::stacktrace)
endif()

if (GLOBED_COMPILE_LIBS)
    CPMAddPackage("gh:dankmeme01/libsodium-cmake#226abba")
    CPMAddPackage("gh:xiph/opus#v1.5.2")

    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(sodium PRIVATE "-Wno-inaccessible-base" "-Wno-pointer-sign" "-Wno-user-defined-warnings")
    endif()

    target_link_libraries(${PROJECT_NAME} sodium opus)
else()
    CPMAddPackage(
        NAME sodium
        GIT_REPOSITORY "https://github.com/jedisct1/libsodium.git"
        GIT_TAG "1.0.20-RELEASE"
    )
    CPMAddPackage(
        NAME opus
        GIT_REPOSITORY "https://github.com/xiph/opus.git"
        GIT_TAG "v1.5.2"
        DOWNLOAD_ONLY YES
    )

    add_library(sodium STATIC IMPORTED)
    set_target_properties(sodium PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/libs/sodium/libsodium.lib")
    add_library(opus STATIC IMPORTED)
    set_target_properties(opus PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/libs/opus/opus.lib")

    target_compile_definitions(${PROJECT_NAME} PRIVATE SODIUM_STATIC=1)
    target_include_directories(${PROJECT_NAME} PRIVATE "${sodium_SOURCE_DIR}/src/libsodium/include")
    target_include_directories(${PROJECT_NAME} PRIVATE "${opus_SOURCE_DIR}/include")

    target_link_libraries(${PROJECT_NAME} sodium opus)
endif()

if (WIN32)
    target_link_libraries(${PROJECT_NAME} ws2_32 crypt32)
endif()

if (APPLE)
    find_library(IOKIT_LIBRARY IOKit)
    target_link_libraries(${PROJECT_NAME} ${IOKIT_LIBRARY})
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
