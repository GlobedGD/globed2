/*
* Do not touch this file unless you are adding new functionality or fixing something.
* If you want to change configuration, do it in `config.hpp`.
*
* This file contains various useful macros to try and decrease the spaghetti code.
*/

#pragma once
#include <config.hpp>

// read the contributor guide for more info on GLOBED_ROOT_NO_GEODE
#ifndef GLOBED_ROOT_NO_GEODE
# include <Geode/Geode.hpp>
#endif

#include <cassert>
#include <stdexcept>
#include <bit>
#include <mutex> // std::call_once and std::once_flag for singletons
#include <memory> // std::unique_ptr for singletons

/* platform-specific:
* GLOBED_HAS_FMOD - 0 or 1, whether this platform links to FMOD
* GLOBED_HAS_DRPC - 0 or 1, whether this platform can use discord rich presence
*/

#ifndef GLOBED_ROOT_NO_GEODE
# ifdef GEODE_IS_WINDOWS
#  define GLOBED_HAS_FMOD GLOBED_FMOD_WINDOWS
#  define GLOBED_HAS_DRPC GLOBED_DRPC_WINDOWS
# elif defined(GEODE_IS_MACOS)
#  define GLOBED_HAS_FMOD GLOBED_FMOD_MAC
#  define GLOBED_HAS_DRPC GLOBED_DRPC_MAC
# elif defined(GEODE_IS_ANDROID)
#  define GLOBED_HAS_FMOD GLOBED_FMOD_ANDROID
#  define GLOBED_HAS_DRPC GLOBED_DRPC_ANDROID
# else
#  error "Unsupported system. This mod only supports Windows, Mac and Android."
# endif
#endif

/*
* GLOBED_CAN_USE_SOURCE_LOCATION - 0 or 1, whether <source_location> header is available
*/

// force consteval for source location
#if GLOBED_FORCE_CONSTEVAL && !defined(__cpp_consteval)
# define __cpp_consteval 1
#endif // GLOBED_FORCE_CONSTEVAL && !defined(__cpp_consteval)

#if defined(__cpp_consteval)
# define GLOBED_CAN_USE_SOURCE_LOCATION 1
# include <source_location>
# define GLOBED_SOURCE std::source_location::current()
#else
# define GLOBED_CAN_USE_SOURCE_LOCATION 0
#endif // defined(__cpp_consteval)

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

#define GLOBED_UNIMPL(message) GLOBED_ASSERT(false, std::string("unimplemented: ") + (message))

constexpr bool GLOBED_LITTLE_ENDIAN = std::endian::native == std::endian::little;

// singleton classes

#define GLOBED_SINGLETON(cls) \
    private: \
    static std::once_flag __singleton_once_flag; \
    static std::unique_ptr<cls> __singleton_instance; \
    \
    public: \
    static inline cls& get() { \
        std::call_once(__singleton_once_flag, []() { \
            __singleton_instance = std::make_unique<cls>(); \
        }); \
        return *__singleton_instance; \
    } \
    cls(const cls&) = delete; \
    cls& operator=(const cls&) = delete;

#define GLOBED_SINGLETON_DEF(cls) \
    std::unique_ptr<cls> cls::__singleton_instance = std::unique_ptr<cls>(nullptr); \
    std::once_flag cls::__singleton_once_flag;
