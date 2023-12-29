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

# Windows - Include winsock
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_compile_definitions(WIN32_LEAN_AND_MEAN=1) # geode moment
    add_definitions(/FI"WinSock2.h")
endif()

# i am crying so hard right now
if (CMAKE_HOST_SYSTEM MATCHES "Linux" AND CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_compile_options("-march=skylake")
endif()

# Android - enable exceptions and set various options
if (ANDROID)
    # TODO this does nothing vvvvv
    set(ANDROID_PLATFORM "android-24") # android 7, `fseeko` does not exist on earlier sdk versions and is needed for cURL
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions") # (this doesn't work !!!!!!! fix me!!!)
endif()

add_library(${PROJECT_NAME} SHARED ${SOURCES})

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

target_link_libraries(${PROJECT_NAME} UIBuilder opus sodium) # TODO libcurl on mac

# Android - link to fmod and libcurl
if (ANDROID)
    target_link_libraries(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/libs/libfmod.so $ENV{GEODE_SDK}/loader/include/link/android32/libcurl.a) # TODO replace with the proper fmod, and armv8 too
endif()

# Windows - link to libcurl
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(${PROJECT_NAME} $ENV{GEODE_SDK}/loader/include/link/libcurl.lib)
endif()

# TODO mac - link to libcurl

setup_geode_mod(${PROJECT_NAME})
