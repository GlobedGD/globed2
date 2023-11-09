# I am horrendous at organizing cmakelists. Please help me.
cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "x86_64")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(globed2 VERSION 1.0.0)

file(GLOB_RECURSE SOURCES
	src/*.cpp
)

# Include winsock
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    if(MSVC)
        add_definitions(/FI"winsock2.h")
    else()
        # GCC or Clang
        add_definitions(-include winsock2.h)
    endif()
endif()

# Include our libs
include_directories(libs/libsodium/include libs/opus/include)

# lib files
set(LIBSODIUM_DIR "${CMAKE_SOURCE_DIR}/libs/libsodium")
set(OPUS_DIR "${CMAKE_SOURCE_DIR}/libs/opus")

add_definitions(-DSODIUM_STATIC=1)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_definitions(-DSODIUM_EXPORT=)
    
    find_library(LIBSODIUM_LIBRARY NAMES libsodium HINTS ${LIBSODIUM_DIR}/Win32)
    find_library(OPUS_LIBRARY NAMES opus HINTS ${OPUS_DIR}/Win32)
    
elseif(ANDROID)
    # todo x64 vs x32
    set(LIBSODIUM_LIBRARY ${LIBSODIUM_DIR}/android-armv7/libsodium.a)
    set(OPUS_LIBRARY ${OPUS_DIR}/android-armv7/libopus.a)
endif()

message(STATUS "Libsodium: ${LIBSODIUM_LIBRARY}")
message(STATUS "Opus: ${OPUS_LIBRARY}")

# disable iterator debugging if building in debug
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(_HAS_ITERATOR_DEBUGGING=0)
endif()

add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# UIBuilder
CPMAddPackage("gh:camila314/UIBuilder#main")
target_link_libraries(${PROJECT_NAME} UIBuilder)

# done so you can include root files with <file.hpp>
target_include_directories(${PROJECT_NAME} PRIVATE src/)

# link to our libraries
target_link_libraries(${PROJECT_NAME} ${LIBSODIUM_LIBRARY} ${OPUS_LIBRARY})

# link to fmod on android
if (ANDROID)
    target_link_libraries(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/libs/libfmod.so)
endif()

setup_geode_mod(${PROJECT_NAME})
