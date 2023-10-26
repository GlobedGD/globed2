cmake_minimum_required(VERSION 3.27)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set up your test target
project(globed_tests VERSION 1.0.0)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR})

add_definitions(-include ../testdef.hpp)
add_executable(globed_tests tests.cpp
    ../src/crypto/box.cpp
    ../src/data/bytebuffer.cpp
    ../src/util/data.cpp
    ../src/util/rng.cpp
    ../src/util/time.cpp
    ../src/util/debugging.cpp
)

enable_testing()

# Include winsock
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    if(MSVC)
        add_definitions(/FI"winsock2.h")
    else()
        # GCC or Clang
        add_definitions(-include winsock2.h)
    endif()
endif()

# Include libsodium
find_package(Sodium REQUIRED)
if(Sodium_FOUND)
    include_directories(${sodium_INCLUDE_DIRS})
    target_link_libraries(globed_tests ${sodium_LIBRARIES})
    message(STATUS "Found sodium: ${Sodium_LIBRARIES}")
else()
    message(FATAL_ERROR "libsodium not found. Make sure it's installed.")
endif()

# done so you can include root files with <file.hpp>
target_include_directories(globed_tests PRIVATE ../src/)

# Register the test with CTest
add_test(NAME GlobedTests COMMAND globed_tests)
