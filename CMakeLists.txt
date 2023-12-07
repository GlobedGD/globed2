cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "x86_64")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(globed2 VERSION 1.0.0)

option(ENABLE_DEBUG "Debug mode" OFF)
if (ENABLE_DEBUG)
    add_compile_definitions(GLOBED_DEBUG=1)
else()
    # Enable LTO in release (2.5x less binary size, no noticable compile time hit)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

file(GLOB_RECURSE SOURCES
	src/*.cpp
)

# Include winsock
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_compile_definitions(WIN32_LEAN_AND_MEAN=1) # geode moment
    add_definitions(/FI"WinSock2.h")
endif()

# i am crying so hard right now
if (CMAKE_HOST_SYSTEM MATCHES "Linux" AND CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_compile_options("-march=skylake")
endif()

# enable exceptions on android
if (ANDROID)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")
endif()

add_library(${PROJECT_NAME} SHARED ${SOURCES})

# enable extra warnings
if (CMAKE_HOST_SYSTEM STREQUAL "Windows")
else()
    # i hate this language okay?
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-c++11-compat -Wno-c++14-compat -Wno-c++17-compat -Wno-old-style-cast -Wno-implicit-int-float-conversion -Wno-global-constructors -Wno-pre-c++20-compat-pedantic -Wno-exit-time-destructors -Wno-reserved-identifier -Wno-reserved-macro-identifier -Wno-dollar-in-identifier-extension -Wno-ctad-maybe-unsupported -Wno-unsafe-buffer-usage -Wno-newline-eof -Wno-shadow -Wno-inconsistent-missing-destructor-override -Wno-float-conversion -Wno-shorten-64-to-32 -Wno-sign-conversion -Wno-suggest-destructor-override -Wno-suggest-override -Wno-overloaded-virtual -Wno-unused-parameter -Wno-undefined-func-template -Wno-non-virtual-dtor -Wno-sign-compare -Wno-header-hygiene)
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
CPMAddPackage("gh:camila314/UIBuilder#main")
CPMAddPackage("gh:xiph/opus#master")
CPMAddPackage("gh:dankmeme01/libsodium-cmake#master")

# TODO suppress -Wpointer-sign from libsodium
target_link_libraries(${PROJECT_NAME} UIBuilder opus sodium)

# link to fmod on android
if (ANDROID)
    target_link_libraries(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/libs/libfmod.so)
endif()

setup_geode_mod(${PROJECT_NAME})
