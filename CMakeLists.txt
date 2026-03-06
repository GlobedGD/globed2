cmake_minimum_required(VERSION 3.21)
cmake_policy(SET CMP0069 NEW)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()

project(globed2 VERSION 1.0.0)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

# these have to come early, cant be in geobuild
set(ARC_FEATURE_NET ON CACHE BOOL "" FORCE)
set(GEODE_CODEGEN_EXTRA_ARGS "--skip-pugixml" CACHE STRING "" FORCE)
set(GEODE_ASP_VERSION "#8f03a7c" CACHE STRING "" FORCE)
set(GEODE_NO_PUGIXML_HEADER ON CACHE BOOL "" FORCE)
add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

CPMAddPackage("gh:dankmeme01/geobuild#cf2207a")
include("${CMAKE_CURRENT_BINARY_DIR}/geobuild-gen.cmake")
