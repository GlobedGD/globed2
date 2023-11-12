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

message(STATUS "system: ${CMAKE_HOST_SYSTEM}")

# Include winsock
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    if(MSVC)
        add_compile_definitions(WIN32_LEAN_AND_MEAN=1)
        add_definitions(/FI"winsock2.h")
    else()
        # GCC or Clang
        add_definitions(-include winsock2.h)
    endif()
endif()

# i am crying so hard right now
if (CMAKE_HOST_SYSTEM MATCHES "Linux")
    add_compile_options("-march=skylake")
endif()

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

CPMAddPackage("gh:camila314/UIBuilder#main")
target_link_libraries(${PROJECT_NAME} UIBuilder)

CPMAddPackage("gh:xiph/opus#master")
target_link_libraries(${PROJECT_NAME} opus)

CPMAddPackage("gh:dankmeme01/libsodium-cmake#a30cfce")
target_link_libraries(${PROJECT_NAME} sodium)
# target_include_directories(${PROJECT_NAME} PRIVATE ${libsodium_SOURCE_DIR}/src/libsodium/include)

# done so you can include root files with <file.hpp>
target_include_directories(${PROJECT_NAME} PRIVATE src/)

# link to fmod on android
if (ANDROID)
    target_link_libraries(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/libs/libfmod.so)
endif()

setup_geode_mod(${PROJECT_NAME})
