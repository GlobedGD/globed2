cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(GLOBED_IOS ON)
endif()

if (GLOBED_IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()

project(globed2 VERSION 1.0.0)

# Options
option(GLOBED_VOICE_SUPPORT "Enable voice chat support" ON)
option(GLOBED_DEBUG "Enable debug mode" OFF)
option(GLOBED_DEBUG_INTERPOLATION "Enable interpolation debugging, adds a TON of extra logs, and some visual elements" OFF)
option(GLOBED_MODULE_GLOBAL_TRIGGERS "Global triggers module" OFF)

# Configure platform sources
if (ANDROID)
    file(GLOB_RECURSE PLATFORM_SOURCES CONFIGURE_DEPENDS "src/platform/android/*.cpp")
elseif (APPLE AND GLOBED_IOS)
    file(GLOB_RECURSE PLATFORM_SOURCES CONFIGURE_DEPENDS "src/platform/ios/*.cpp")
elseif (APPLE)
    file(GLOB_RECURSE PLATFORM_SOURCES CONFIGURE_DEPENDS "src/platform/macos/*.cpp")
elseif (WIN32)
    file(GLOB_RECURSE PLATFORM_SOURCES CONFIGURE_DEPENDS "src/platform/windows/*.cpp")
else()
    message(FATAL_ERROR "Unsupported platform!")
endif()

# Configure module sources
set(MODULE_SOURCES "")

if (GLOBED_MODULE_GLOBAL_TRIGGERS)
    file(GLOB_RECURSE GLOBAL_TRIGGERS_SOURCES CONFIGURE_DEPENDS "src/modules/global-triggers/*.cpp")
    list(APPEND MODULE_SOURCES ${GLOBAL_TRIGGERS_SOURCES})
endif()

# Configure rest of sources
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    src/core/*.cpp
    src/util/*.cpp
    src/ui/*.cpp
    ${PLATFORM_SOURCES}
    ${MODULE_SOURCES}
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_include_directories(globed2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(globed2 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

setup_geode_mod(${PROJECT_NAME})

if (GLOBED_DEBUG)
    target_compile_definitions(${PROJECT_NAME} PUBLIC GLOBED_DEBUG)
    set(QUNET_DEBUG ON)
    message(STATUS "Building Globed with debug support")
endif()

if (GLOBED_DEBUG_INTERPOLATION)
    target_compile_definitions(${PROJECT_NAME} PUBLIC GLOBED_DEBUG_INTERPOLATION)
endif()

# Dependencies
CPMAddPackage(
    NAME capnproto
    GIT_REPOSITORY "https://github.com/capnproto/capnproto.git"
    GIT_TAG "v1.2.0"
    OPTIONS "CAPNP_LITE ON"
            "BUILD_TESTING OFF"
            "WITH_FIBERS OFF"
)

CPMAddPackage("gh:GlobedGD/server-shared#main")
CPMAddPackage("gh:dankmeme01/qunet-cpp#main")
CPMAddPackage("gh:dankmeme01/uibuilder#456706b")
CPMAddPackage("gh:dankmeme01/cue#main")
CPMAddPackage("gh:GlobedGD/argon@1.1.9")
CPMAddPackage("gh:Prevter/sinaps#2541d6d")

if (GLOBED_VOICE_SUPPORT)
    CPMAddPackage("gh:xiph/opus#v1.5.2")
    target_compile_definitions(${PROJECT_NAME} PUBLIC GLOBED_VOICE_SUPPORT)
    target_link_libraries(${PROJECT_NAME} opus)
endif()

target_link_libraries(${PROJECT_NAME} CapnProto::capnp ServerShared qunet asp argon cue UIBuilder sinaps)
