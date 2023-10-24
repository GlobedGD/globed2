/*
* Do not touch this file unless you are adding new functionality or fixing something.
* If you want to change something, do it in `config.hpp`.
*
* This file contains various useful macros to try and decrease the spaghetti code.
*/

#pragma once
#include <config.hpp>
#include <Geode/Geode.hpp>

#include <cassert>
#include <bit>

/* platform-specific:
* GLOBED_HAS_FMOD - 0 or 1, whether this platform links to FMOD
* GLOBED_HAS_DRPC - 0 or 1, whether this platform can use discord rich presence
*/

#ifdef GEODE_IS_WINDOWS
# define GLOBED_HAS_FMOD GLOBED_FMOD_WINDOWS
# define GLOBED_HAS_DRPC 1
#elif defined(GEODE_IS_MACOS)
# define GLOBED_HAS_FMOD GLOBED_FMOD_MAC
# define GLOBED_HAS_DRPC 0
#elif defined(GEODE_IS_ANDROID)
# define GLOBED_HAS_FMOD GLOBED_FMOD_ANDROID
# define GLOBED_HAS_DRPC 0
#else
# error "Unsupported system. This mod only supports Windows, Mac and Android."
#endif

/* platform independent:
* GLOBED_CAN_USE_SOURCE_LOCATION - 0 or 1, whether <source_location> header is available
* GLOBED_ASSERT(cond,msg) - throw a runtime error if assertion fails
* GLOBED_LITTLE_ENDIAN - constexpr bool, self describing
*/

// source_location
#if defined(__cpp_consteval) && GLOBED_USE_SOURCE_LOCATION
# define GLOBED_CAN_USE_SOURCE_LOCATION 1
# include <source_location>
# define GLOBED_SOURCE std::source_location::current()
#else
# define GLOBED_CAN_USE_SOURCE_LOCATION 0
#endif

// throws a runtime error if assertion fails
#define GLOBED_ASSERT(condition,message) if (!(condition)) { geode::log::error("Assertion failed: {}", message); throw std::runtime_error(std::string("Globed assertion failed: ") + message); }

constexpr bool GLOBED_LITTLE_ENDIAN = std::endian::native == std::endian::little;