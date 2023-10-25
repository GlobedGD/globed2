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
#include <stdexcept>
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

/*
* GLOBED_ASSERT - throws a runtime error if assertion fails
* GLOBED_HARD_ASSERT - terminates the program if assertion fails. Don't use it unless the condition indicates a logic error in the code.
*/
#if GLOBED_CAN_USE_SOURCE_LOCATION
# define GLOBED_ASSERT(condition,message) \
    if (!(condition)) { \
        auto __ev_msg = message; \
        auto loc = GLOBED_SOURCE; \
        geode::log::error("Assertion failed at {}: {}", fmt::format("{}:{} ({})", loc.file_name(), loc.line(), loc.function_name()), __ev_msg); \
        throw std::runtime_error(std::string("Globed assertion failed: ") + __ev_msg); \
    }
# define GLOBED_HARD_ASSERT(condition,message) \
    if (!(condition)) { \
        auto __ev_msg = message; \
        auto loc = GLOBED_SOURCE; \
        geode::log::error("Assertion failed at {}: {}", fmt::format("{}:{} ({})", loc.file_name(), loc.line(), loc.function_name()), __ev_msg); \
        std::abort(); \
    }
#else
# define GLOBED_ASSERT(condition,message) \
    if (!(condition)) { \
        auto __ev_msg = message; \
        geode::log::error("Assertion failed: {}", __ev_msg); \
        throw std::runtime_error(std::string("Globed assertion failed: ") + __ev_msg); \
    }
# define GLOBED_HARD_ASSERT(condition,message) \
    if (!(condition)) { \
        auto __ev_msg = message; \
        geode::log::error("Assertion failed: {}", __ev_msg); \
        std::abort(); \
    }
#endif

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