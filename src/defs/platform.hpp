#pragma once
#include <config.hpp>
#include <bit>

/*
* platform macros
* GLOBED_IS_UNIX - unix-like system

* GLOBED_PLATFORM_STRING_PLATFORM - string in format like "Mac", "Android", "Windows"
* GLOBED_PLATFORM_STRING_ARCH - string in format like "x86", "x64", "armv7", "arm64"
* GLOBED_PLATFORM_STRING - those two above combined into one, i.e. "Windows x86"
*/

#if defined(GEODE_IS_MACOS) || defined(GEODE_IS_ANDROID)
# define GLOBED_IS_UNIX 1
#endif

#ifdef GEODE_IS_MACOS
# define GLOBED_PLATFORM_STRING_PLATFORM "Mac"
# define GLOBED_PLATFORM_STRING_ARCH "x86_64"
#elif defined(GEODE_IS_WINDOWS)
# define GLOBED_PLATFORM_STRING_PLATFORM "Windows"
# define GLOBED_PLATFORM_STRING_ARCH "x86"
#elif defined(GEODE_IS_ANDROID)
# define GLOBED_PLATFORM_STRING_PLATFORM "Android"
# ifdef GEODE_IS_ANDROID64
#  define GLOBED_PLATFORM_STRING_ARCH "arm64-v8a"
# else
#  define GLOBED_PLATFORM_STRING_ARCH "armeabi-v7a"
# endif
#endif

#define GLOBED_PLATFORM_STRING GLOBED_PLATFORM_STRING_PLATFORM " " GLOBED_PLATFORM_STRING_ARCH

/* platform-specific:
* GLOBED_HAS_FMOD - 0 or 1, whether this platform links to FMOD
* GLOBED_HAS_DRPC - 0 or 1, whether this platform can use discord rich presence
*/

#ifdef GEODE_IS_WINDOWS
# define GLOBED_HAS_FMOD GLOBED_FMOD_WINDOWS
# define GLOBED_HAS_DRPC GLOBED_DRPC_WINDOWS
# define GLOBED_HAS_KEYBINDS 1
#elif defined(GEODE_IS_MACOS)
# define GLOBED_HAS_FMOD GLOBED_FMOD_MAC
# define GLOBED_HAS_DRPC GLOBED_DRPC_MAC
# define GLOBED_HAS_KEYBINDS 0
#elif defined(GEODE_IS_ANDROID)
# define GLOBED_HAS_FMOD GLOBED_FMOD_ANDROID
# define GLOBED_HAS_DRPC GLOBED_DRPC_ANDROID
# define GLOBED_HAS_KEYBINDS 0
#else
# error "what"
#endif

#ifdef GLOBED_DISABLE_CUSTOM_KEYBINDS
# undef GLOBED_HAS_KEYBINDS
# define GLOBED_HAS_KEYBINDS 0
#endif

#if GLOBED_HAS_FMOD && !defined(GLOBED_DISABLE_VOICE_SUPPORT)
# define GLOBED_VOICE_SUPPORT 1
#else
# define GLOBED_VOICE_SUPPORT 0
#endif

constexpr bool GLOBED_LITTLE_ENDIAN = std::endian::native == std::endian::little;