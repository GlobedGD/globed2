cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "x86_64")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(globed2 VERSION 1.0.0)

# Debug/Release options
option(ENABLE_DEBUG "Debug mode" OFF)
if (ENABLE_DEBUG OR CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(GLOBED_DEBUG=1)
    add_compile_definitions(GEODE_DEBUG=1)
else()
    # Enable LTO in release (2.5x less binary size, costs only a few extra seconds of build time)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

file(GLOB_RECURSE SOURCES
	src/*.cpp
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

# Windows - Include winsock
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_compile_definitions(WIN32_LEAN_AND_MEAN=1) # geode moment
    if (MSVC)
        add_definitions(/FI"WinSock2.h")
    else()
        add_definitions("-include WinSock2.h")
        set(GLOBED_WINDOWS_CLANG ON)
    endif()
endif()

# Clang/GCC - suppress some warnings
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_definitions("-Wno-inaccessible-base")
    add_definitions("-Wno-pointer-sign")

    # thank you hjfod
    add_definitions("-Wno-deprecated-declarations")
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
CPMAddPackage("gh:xiph/opus#master")
CPMAddPackage("gh:dankmeme01/libsodium-cmake#master")

# Windows Clang - ensure 32-bitness for opus and libsodium and enable additional instruction sets
if (GLOBED_WINDOWS_CLANG)
    target_compile_options(opus PRIVATE "-m32")
    target_compile_options(sodium PRIVATE "-m32" "-maes" "-mpclmul" "-msse4.1")
endif()

target_link_libraries(${PROJECT_NAME} UIBuilder opus sodium)

setup_geode_mod(${PROJECT_NAME})
