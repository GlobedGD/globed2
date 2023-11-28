#pragma once
#include "basic.hpp"
#include <stdint.h>
#include <bit>

/*
* platform macros
* GLOBED_WIN32, GLOBED_MAC, GLOBED_ANDROID - self descriptive
* GLOBED_UNIX - mac or android
* GLOBED_X86_32, GLOBED_X86_64, GLOBED_ARM32, GLOBED_ARM64 - only 1 of those 4 is defined, indicating if we are 32-bit or 64-bit
* GLOBED_X86, GLOBED_ARM - any x86 or arm
*
* GLOBED_PLATFORM_STRING_PLATFORM - string in format like "Mac", "Android", "Windows"
* GLOBED_PLATFORM_STRING_ARCH - string in format like "x86", "x64", "armv7", "arm64"
* GLOBED_PLATFORM_STRING - those two above combined into one, i.e. "Windows x86"
*/

#if defined(_WIN32) || defined(_WIN64)
# define GLOBED_WIN32 1
# define GLOBED_PLATFORM_STRING_PLATFORM "Windows"
#elif defined(__APPLE__)
# define GLOBED_MAC 1
# define GLOBED_UNIX 1
# define GLOBED_PLATFORM_STRING_PLATFORM "Mac"
#elif defined(__ANDROID__)
# define GLOBED_ANDROID 1
# define GLOBED_UNIX 1
# define GLOBED_PLATFORM_STRING_PLATFORM "Android"
#elif defined(GLOBED_TESTING)
# define GLOBED_UNIX 1
# define GLOBED_PLATFORM_STRING_PLATFORM "N/A"
#else
# error "Unknown platform"
#endif

#if defined(__i386__) || defined(_M_IX86) || defined(_M_X64) || defined(__x86_64__)
# define GLOBED_X86 1
# if UINTPTR_MAX > 0xffffffff
#  define GLOBED_X86_64 1
#  define GLOBED_PLATFORM_STRING_ARCH "x64"
# else
#  define GLOBED_X86_32 1
#  define GLOBED_PLATFORM_STRING_ARCH "x32"
# endif
#elif defined(__aarch64__) || defined(_M_ARM64)
# define GLOBED_ARM 1
# define GLOBED_ARM64 1
# define GLOBED_PLATFORM_STRING_ARCH "arm64"
#elif defined(__ARM_ARCH_7S__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__) || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__) || defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__)
# define GLOBED_ARM 1
# define GLOBED_ARM32 1
# define GLOBED_PLATFORM_STRING_ARCH "armv7"
#else
# error "Unknown architecture"
#endif

#define GLOBED_PLATFORM_STRING GLOBED_PLATFORM_STRING_PLATFORM " " GLOBED_PLATFORM_STRING_ARCH

/* platform-specific:
* GLOBED_HAS_FMOD - 0 or 1, whether this platform links to FMOD
* GLOBED_HAS_DRPC - 0 or 1, whether this platform can use discord rich presence
*/

#ifdef GLOBED_WIN32
# define GLOBED_HAS_FMOD GLOBED_FMOD_WINDOWS
# define GLOBED_HAS_DRPC GLOBED_DRPC_WINDOWS
# define GLOBED_HAS_KEYBINDS 1
#elif defined(GLOBED_MAC)
# define GLOBED_HAS_FMOD GLOBED_FMOD_MAC
# define GLOBED_HAS_DRPC GLOBED_DRPC_MAC
# define GLOBED_HAS_KEYBINDS 1
#elif defined(GLOBED_ANDROID)
# define GLOBED_HAS_FMOD GLOBED_FMOD_ANDROID
# define GLOBED_HAS_DRPC GLOBED_DRPC_ANDROID
# define GLOBED_HAS_KEYBINDS 0
#elif defined(GLOBED_TESTING)
#else
# error "what"
#endif

#if GLOBED_HAS_FMOD && !defined(GLOBED_DISABLE_VOICE_SUPPORT)
# define GLOBED_VOICE_SUPPORT 1
#else
# define GLOBED_VOICE_SUPPORT 0
#endif

constexpr bool GLOBED_LITTLE_ENDIAN = std::endian::native == std::endian::little;