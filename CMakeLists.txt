cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(globed2 VERSION 1.0.0)

if (${CMAKE_SYSTEM_NAME} STREQUAL "iOS")
    unset(CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0")
    set(CMAKE_SYSTEM_NAME "iOS")
else()
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

if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR "${CMAKE_BUILD_TYPE}asdf" STREQUAL "asdf" OR ENABLE_DEBUG)
    set(GLOBED_IS_DEBUG ON)
endif()

if (GLOBED_IS_DEBUG)
    add_compile_definitions(GLOBED_DEBUG=1)
    add_compile_definitions(GEODE_DEBUG=1)
    add_compile_definitions(ASP_ENABLE_DEBUG=1)
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

if (CMAKE_HOST_SYSTEM MATCHES "Linux" AND CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions(${PROJECT_NAME} PRIVATE GLOBED_LINUX_COMPILATION=1)
endif()

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

target_include_directories(${PROJECT_NAME} PRIVATE src/)
target_include_directories(${PROJECT_NAME} PRIVATE libs/)

CPMAddPackage("gh:camila314/uibuilder#09c05a5")
CPMAddPackage(
    NAME Boost
    VERSION 1.87.0
    URL https://github.com/boostorg/boost/releases/download/boost-1.87.0/boost-1.87.0-cmake.7z
    URL_HASH SHA256=61d61a3f33283bab8f3736745a4e47c37141f07e413986cd05d1f942c69d529a
    OPTIONS "BOOST_ENABLE_CMAKE ON" "BOOST_INCLUDE_LIBRARIES describe\\\;stacktrace\\\;thread"
)
CPMAddPackage("gh:dankmeme01/asp2#2378a82")

if (WIN32)
    if (GLOBED_IS_DEBUG)
        target_compile_definitions(asp PRIVATE _HAS_ITERATOR_DEBUGGING=0)
    endif()

    target_compile_definitions(asp PRIVATE ASP_ENABLE_FORMAT=1)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ASP_ENABLE_FORMAT=1)
endif()

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

    set(SODIUM_VERSION_IN "${CMAKE_CURRENT_SOURCE_DIR}/libs/sodium/version.h")
    set(SODIUM_VERSION_OUT "${sodium_SOURCE_DIR}/src/libsodium/include/sodium/")
    if (NOT EXISTS "${SODIUM_VERSION_OUT}/version.h")
        file(COPY "${SODIUM_VERSION_IN}" DESTINATION "${SODIUM_VERSION_OUT}")
    endif()

	target_link_libraries(${PROJECT_NAME} sodium opus)
endif()

if (WIN32)
    target_link_libraries(${PROJECT_NAME} ws2_32)
endif()

if (APPLE)
    find_library(IOKIT_LIBRARY IOKit)
    target_link_libraries(${PROJECT_NAME} ${IOKIT_LIBRARY})
endif()

CPMAddPackage(
    NAME curl
    GIT_REPOSITORY "https://github.com/curl/curl.git"
    GIT_TAG "curl-8_11_1"
    DOWNLOAD_ONLY YES
)

add_library(curl STATIC IMPORTED)

if (ANDROID)
    if (ANDROID_ABI STREQUAL "arm64-v8a")
        set(LIB_PLATFORM "android64")
    else()
        set(LIB_PLATFORM "android32")
    endif()
elseif (WIN32)
    set(LIB_PLATFORM "win64")
else()
    set(LIB_PLATFORM "macos")
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE CURL_STATICLIB=1)

if (WIN32)
    set_target_properties(curl PROPERTIES IMPORTED_LOCATION "$ENV{GEODE_SDK}/loader/include/link/${LIB_PLATFORM}/libcurl.lib")
else()
    set_target_properties(curl PROPERTIES IMPORTED_LOCATION "$ENV{GEODE_SDK}/loader/include/link/${LIB_PLATFORM}/libcurl.a")
endif()

target_include_directories(${PROJECT_NAME} PRIVATE "${curl_SOURCE_DIR}/include")

target_link_libraries(${PROJECT_NAME} curl)

if (APPLE)
	target_link_libraries(${PROJECT_NAME} z)
elseif (WIN32)
    target_link_libraries(${PROJECT_NAME} crypt32)
elseif (ANDROID)
    target_link_libraries(${PROJECT_NAME} android)
endif()

if (GLOBED_OSS_BUILD)
    message(STATUS "Building open-source version, not linking to bb")
    target_compile_definitions(${PROJECT_NAME} PRIVATE GLOBED_OSS_BUILD=1)
else()
    if (WIN32)
        target_link_libraries(${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/libs/bb/bb.lib")
        target_link_libraries(${PROJECT_NAME} ntdll.lib userenv.lib runtimeobject.lib Iphlpapi.lib)
    else ()
        target_link_libraries(${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/libs/bb/bb-${LIB_PLATFORM}.a")
    endif()
endif()

setup_geode_mod(${PROJECT_NAME})
