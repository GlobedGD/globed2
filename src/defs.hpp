/*
* Do not touch this file unless you are adding new functionality or fixing something.
* If you want to change something, do it in `config.hpp`.
*
* This file contains various useful macros to try and decrease the spaghetti code.
*/

#pragma once
#include <config.hpp>

// if you want to run any tests, you have to define GLOBED_ROOT_NO_GEODE
// as you can't really link to geode without making it a proper mod.
// make sure to also define GLOBED_ASSERT_LOG as a replacement to geode::log::error
#ifndef GLOBED_ROOT_NO_GEODE
# include <Geode/Geode.hpp>
#endif

#include <cassert>
#include <stdexcept>
#include <bit>

/* platform-specific:
* GLOBED_HAS_FMOD - 0 or 1, whether this platform links to FMOD
* GLOBED_HAS_DRPC - 0 or 1, whether this platform can use discord rich presence
*/

#ifndef GLOBED_ROOT_NO_GEODE
# ifdef GEODE_IS_WINDOWS
#  define GLOBED_HAS_FMOD GLOBED_FMOD_WINDOWS
#  define GLOBED_HAS_DRPC 1
# elif defined(GEODE_IS_MACOS)
#  define GLOBED_HAS_FMOD GLOBED_FMOD_MAC
#  define GLOBED_HAS_DRPC 0
# elif defined(GEODE_IS_ANDROID)
#  define GLOBED_HAS_FMOD GLOBED_FMOD_ANDROID
#  define GLOBED_HAS_DRPC 0
# else
#  error "Unsupported system. This mod only supports Windows, Mac and Android."
# endif
#endif

/* platform independent:
* GLOBED_CAN_USE_SOURCE_LOCATION - 0 or 1, whether <source_location> header is available
* GLOBED_ASSERT(cond,msg) - assertions (see below)
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

/*
* GLOBED_ASSERT - throws a runtime error if assertion fails
* GLOBED_HARD_ASSERT - terminates the program if assertion fails. Don't use it unless the condition indicates a logic error in the code.
* GLOBED_UNIMPL - throws a runtime error as the method was not implemented and isn't meant to be called
*/
#if GLOBED_CAN_USE_SOURCE_LOCATION && !defined(GLOBED_ROOT_NO_GEODE)
# define GLOBED_ASSERT(condition,message) \
    if (!(condition)) { \
        auto ev_msg = message; \
        auto loc = GLOBED_SOURCE; \
        geode::log::error("Assertion failed at {}: {}", fmt::format("{}:{} ({})", loc.file_name(), loc.line(), loc.function_name()), ev_msg); \
        throw std::runtime_error(std::string("Globed assertion failed: ") + ev_msg); \
    }
# define GLOBED_HARD_ASSERT(condition,message) \
    if (!(condition)) { \
        auto ev_msg = message; \
        auto loc = GLOBED_SOURCE; \
        geode::log::error("Assertion failed at {}: {}", fmt::format("{}:{} ({})", loc.file_name(), loc.line(), loc.function_name()), ev_msg); \
        std::abort(); \
    }
#else
# ifndef GLOBED_ROOT_NO_GEODE
#  define GLOBED_ASSERT_LOG geode::log::error
# endif
# define GLOBED_ASSERT(condition,message) \
    if (!(condition)) { \
        auto ev_msg = message; \
        GLOBED_ASSERT_LOG(std::string("Assertion failed: ") + ev_msg); \
        throw std::runtime_error(std::string("Globed assertion failed: ") + ev_msg); \
    }
# define GLOBED_HARD_ASSERT(condition,message) \
    if (!(condition)) { \
        auto ev_msg = message; \
        GLOBED_ASSERT_LOG(std::string("Assertion failed: ") + ev_msg); \
        std::abort(); \
    }
#endif

#define GLOBED_UNIMPL(message) GLOBED_ASSERT(false, "unimplemented: " message)

constexpr bool GLOBED_LITTLE_ENDIAN = std::endian::native == std::endian::little;

// singleton classes

#define GLOBED_SINGLETON(cls) \
    public: \
    static cls& get(); \
    cls(const cls&) = delete; \
    cls& operator=(const cls&) = delete;

#define GLOBED_SINGLETON_GET(cls) \
    cls& cls::get() { \
        static cls instance; \
        return instance; \
    }
