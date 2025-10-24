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
option(GLOBED_QUIC_SUPPORT "Enable QUIC support" ON)
option(GLOBED_ADVANCED_DNS_SUPPORT "Enable advanced DNS support" ON)
option(GLOBED_DEBUG "Enable debug mode" OFF)
option(GLOBED_RELEASE "Enable release mode optimizations" OFF)
option(GLOBED_OSS_BUILD "Open source build that does not require closed-source dependencies" OFF)
option(GLOBED_MODULE_GLOBAL_TRIGGERS "Global triggers module" OFF)
option(GLOBED_MODULE_SCRIPTING "Scripting module" OFF)
option(GLOBED_MODULE_SCRIPTING_UI "Scripting UI module" OFF)
option(GLOBED_MODULE_TCD_HNS "" OFF)
set(GLOBED_DEFAULT_MAIN_SERVER_URL "qunet://globed.dev" CACHE STRING "Default main server URL")

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

if (GLOBED_MODULE_SCRIPTING)
    file(GLOB_RECURSE SCRIPTING_SOURCES CONFIGURE_DEPENDS "src/modules/scripting/*.cpp")
    list(APPEND MODULE_SOURCES ${SCRIPTING_SOURCES})
endif()

if (GLOBED_MODULE_SCRIPTING_UI)
    file(GLOB_RECURSE SCRIPTING_UI_SOURCES CONFIGURE_DEPENDS "src/modules/scripting-ui/*.cpp")
    list(APPEND MODULE_SOURCES ${SCRIPTING_UI_SOURCES})
endif()

if (GLOBED_MODULE_TCD_HNS)
    file(GLOB_RECURSE TCD_HNS_SOURCES CONFIGURE_DEPENDS "src/modules/comm/tcd-hidenseek/*.cpp")
    list(APPEND MODULE_SOURCES ${TCD_HNS_SOURCES})
endif()

if (GLOBED_MODULE_ACTIVE_PLAYER_SWITCH)
    file(GLOB_RECURSE ACTIVE_PLAYER_SWITCH_SOURCES CONFIGURE_DEPENDS "src/modules/comm/active-player-switch/*.cpp")
    list(APPEND MODULE_SOURCES ${ACTIVE_PLAYER_SWITCH_SOURCES})
endif()

file(GLOB_RECURSE DEATHLINK_SOURCES CONFIGURE_DEPENDS "src/modules/deathlink/*.cpp")
list(APPEND MODULE_SOURCES ${DEATHLINK_SOURCES})

file(GLOB_RECURSE TWO_PLAYER_SOURCES CONFIGURE_DEPENDS "src/modules/two-player/*.cpp")
list(APPEND MODULE_SOURCES ${TWO_PLAYER_SOURCES})

file(GLOB_RECURSE UI_SOURCES CONFIGURE_DEPENDS "src/modules/ui/*.cpp")
list(APPEND MODULE_SOURCES ${UI_SOURCES})

# Configure rest of sources
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    src/core/*.cpp
    src/audio/*.cpp
    src/util/*.cpp
    src/ui/*.cpp
    ${PLATFORM_SOURCES}
    ${MODULE_SOURCES}
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_include_directories(globed2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(globed2 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(globed2 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/libs)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

setup_geode_mod(${PROJECT_NAME})

# Setup release stuff

if (GLOBED_RELEASE)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()

# Setup compile definitions

if (GLOBED_DEBUG)
    target_compile_definitions(${PROJECT_NAME} PUBLIC GLOBED_DEBUG)
    set(QUNET_DEBUG ON)
    message(STATUS "Building Globed with debug support")
endif()

if (GLOBED_MODULE_SCRIPTING_UI)
    target_compile_definitions(${PROJECT_NAME} PRIVATE GLOBED_MODULE_SCRIPTING_UI=1)
endif()

if (GLOBED_MODULE_TCD_HNS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE GLOBED_MODULE_TCD_HNS=1)
endif()

if (GLOBED_VOICE_SUPPORT)
    target_compile_definitions(${PROJECT_NAME} PUBLIC GLOBED_VOICE_SUPPORT)

    if (NOT APPLE)
        target_compile_definitions(${PROJECT_NAME} PUBLIC GLOBED_VOICE_CAN_TALK)
    endif()
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE GLOBED_DEFAULT_MAIN_SERVER_URL="${GLOBED_DEFAULT_MAIN_SERVER_URL}")

# Dependencies
CPMAddPackage(
    NAME capnproto
    GIT_REPOSITORY "https://github.com/capnproto/capnproto.git"
    GIT_TAG "v1.2.0"
    OPTIONS "CAPNP_LITE ON"
            "BUILD_TESTING OFF"
            "WITH_FIBERS OFF"
)

CPMAddPackage("gh:GlobedGD/server-shared#76c8d278")
CPMAddPackage(
    NAME qunet-cpp
    GIT_REPOSITORY "https://github.com/dankmeme01/qunet-cpp"
    GIT_TAG "3c21395"
    OPTIONS "QUNET_QUIC_SUPPORT ${GLOBED_QUIC_SUPPORT}"
            "QUNET_ADVANCED_DNS ${GLOBED_ADVANCED_DNS_SUPPORT}"
            "QUNET_DEBUG ${GLOBED_DEBUG}"
)
CPMAddPackage("gh:dankmeme01/uibuilder#456706b") # note: before bumping this, fix NameLabel (and potentially more) where Build(x) is done where x may have a parent sill
CPMAddPackage("gh:dankmeme01/cue#c73aaa0b")
CPMAddPackage("gh:GlobedGD/argon@1.1.9")
CPMAddPackage("gh:Prevter/sinaps#2541d6d")
CPMAddPackage("gh:Prevter/AdvancedLabel#d78d7f82")
# CPMAddPackage("gh:dankmeme01/asp2#c0f3d6ea") # qunet :p

target_compile_definitions(${PROJECT_NAME} PUBLIC UIBUILDER_NO_ARROW=1)

if (GLOBED_VOICE_SUPPORT)
    CPMAddPackage("gh:xiph/opus#v1.5.2")
    target_compile_definitions(${PROJECT_NAME} PUBLIC GLOBED_VOICE_SUPPORT)
    target_link_libraries(${PROJECT_NAME} opus)
endif()

target_link_libraries(${PROJECT_NAME} CapnProto::capnp ServerShared qunet asp argon cue UIBuilder sinaps advanced_label)

if (ANDROID)
    if (ANDROID_ABI STREQUAL "arm64-v8a")
        set(LIB_PLATFORM "android64")
    else()
        set(LIB_PLATFORM "android32")
    endif()
elseif (WIN32)
    set(LIB_PLATFORM "win64")
elseif (GLOBED_IOS)
    set(LIB_PLATFORM "ios")
else()
    set(LIB_PLATFORM "macos")
endif()

# link to bb
if (GLOBED_OSS_BUILD)
    message(STATUS "Building open-source version, not linking to bb")
    target_compile_definitions(${PROJECT_NAME} PRIVATE GLOBED_OSS_BUILD=1)
else()
    if (WIN32)
        target_link_libraries(${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/libs/bb/bb.lib")
        target_link_libraries(${PROJECT_NAME} ntdll.lib userenv.lib runtimeobject.lib Iphlpapi.lib bcrypt.lib)
    else ()
        target_link_libraries(${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/libs/bb/bb-${LIB_PLATFORM}.a")
    endif()
endif()
