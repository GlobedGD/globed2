cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "x86_64")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(globed2 VERSION 1.0.0)

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

# Include winsock
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_compile_definitions(WIN32_LEAN_AND_MEAN=1) # geode moment
    add_definitions(/FI"WinSock2.h")
endif()

# i am crying so hard right now
if (CMAKE_HOST_SYSTEM MATCHES "Linux" AND CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_compile_options("-march=skylake")
endif()

# enable exceptions on android (this doesn't work !!!!!!! fix me!!!)
if (ANDROID)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")
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

# libcurl ssl support
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # on windows we just use schannel (built into windows)
    set(CURL_USE_SCHANNEL ON)
    set(CURL_USE_OPENSSL OFF)
elseif(APPLE)
    # on mac we use secure transport (built into mac)
    set(CURL_USE_SECTRANSP ON)
    set(CURL_CA_PATH none)
    set(CURL_USE_OPENSSL OFF)
elseif(ANDROID)
    # on android we use geode provided openssl library and our own headers (see android-build.md)
    set(CURL_USE_OPENSSL ON)
    set(OPENSSL_ROOT_DIR "${CMAKE_SOURCE_DIR}/libs/openssl")
    set(OPENSSL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/libs/openssl/include")
    set(OPENSSL_CRYPTO_LIBRARY "$ENV{GEODE_SDK}/loader/include/link/android/libcrypto.a")
    set(OPENSSL_SSL_LIBRARY "$ENV{GEODE_SDK}/loader/include/link/android/libssl.a")
endif()

# our favorite libraries
CPMAddPackage("gh:camila314/UIBuilder#main")
CPMAddPackage("gh:xiph/opus#master")
CPMAddPackage("gh:dankmeme01/libsodium-cmake#master")
CPMAddPackage("gh:curl/curl#curl-8_5_0")

target_link_libraries(${PROJECT_NAME} UIBuilder opus sodium libcurl)

# link to fmod on android
if (ANDROID)
    target_link_libraries(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/libs/libfmod.so)
endif()

setup_geode_mod(${PROJECT_NAME})
