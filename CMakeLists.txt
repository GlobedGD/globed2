cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "x86_64")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(globed2 VERSION 1.0.0)

option(GLOBED_NATIVE_ARCH "Native architecture build" OFF)

# Check for debug build
option(ENABLE_DEBUG "Debug mode" OFF)

if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR "${CMAKE_BUILD_TYPE}asdf" STREQUAL "asdf" OR ENABLE_DEBUG)
    set(GLOBED_IS_DEBUG ON)
endif()

# Debug/Release options
if (GLOBED_IS_DEBUG)
    add_compile_definitions(GLOBED_DEBUG=1)
    add_compile_definitions(GEODE_DEBUG=1)
else()
    # Enable LTO in release (2.5x less binary size, costs only a few extra seconds of build time)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# add thingies epending on the current system
if (ANDROID)
    file(GLOB_RECURSE OS_SOURCES "src/platform/os/android/*.cpp" "src/platform/arch/arm/*.cpp")
elseif (APPLE)
    file(GLOB_RECURSE OS_SOURCES "src/platform/os/mac/*.cpp" "src/platform/arch/x86/*.cpp")
elseif (WIN32)
    file(GLOB_RECURSE OS_SOURCES "src/platform/os/windows/*.cpp" "src/platform/arch/x86/*.cpp")
else()
    message(FATAL_ERROR "Unsupported operating system")
endif()

# source files
file(GLOB_RECURSE SOURCES
	src/audio/*.cpp
	src/crypto/*.cpp
	src/data/*.cpp
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

# Windows - Include winsock
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_compile_definitions(WIN32_LEAN_AND_MEAN=1) # geode moment
    if (MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /FIWinSock2.h)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE "-include WinSock2.h")
        set(GLOBED_WINDOWS_CLANG ON)
    endif()

    if (GLOBED_IS_DEBUG)
        message(STATUS "Compiling in debug, disabling iterator debugging")
        add_compile_definitions(_HAS_ITERATOR_DEBUGGING=0)
    endif()
endif()

# i am crying so hard right now
if (CMAKE_HOST_SYSTEM MATCHES "Linux" AND CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_compile_options("-march=skylake")
endif()

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# done so you can include root files with <file.hpp>
target_include_directories(${PROJECT_NAME} PRIVATE src/)

# our favorite libraries
CPMAddPackage("gh:dankmeme01/uibuilder#main")
CPMAddPackage("gh:xiph/opus#main")
CPMAddPackage("gh:dankmeme01/libsodium-cmake#master")
CPMAddPackage(
    NAME Boost
    VERSION 1.84.0
    URL https://github.com/boostorg/boost/releases/download/boost-1.84.0/boost-1.84.0.tar.xz
    URL_HASH SHA256=2e64e5d79a738d0fa6fb546c6e5c2bd28f88d268a2a080546f74e5ff98f29d0e
    OPTIONS "BOOST_ENABLE_CMAKE ON" "BOOST_INCLUDE_LIBRARIES describe" # escape with \\\;
)

if (GLOBED_NATIVE_ARCH)
    # Apply native architecture
    add_compile_options("-march=native")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # Windows Clang - ensure 32-bitness for opus and libsodium and enable additional instruction sets
    # Note - this limits the mod to CPUs that support the AES, pclmul and SSE4.1 instruction sets,
    # and therefore is not used in release. Release builds use MSVC which has a smarter vectorizer.
    target_compile_options(opus PRIVATE "-m32" "-msse4.1")
    target_compile_options(sodium PRIVATE "-m32" "-maes" "-mpclmul" "-msse4.1")
endif()

# Clang/GCC - suppress some warnings
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(sodium PRIVATE "-Wno-inaccessible-base" "-Wno-pointer-sign")

    # thank you hjfod
    target_compile_options(${PROJECT_NAME} PRIVATE "-Wno-deprecated-declarations")
endif()

target_link_libraries(${PROJECT_NAME} UIBuilder opus sodium Boost::describe)

setup_geode_mod(${PROJECT_NAME})
