#pragma once
#include "basic.hpp"
#include <stdexcept>

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

#ifndef GLOBED_ROOT_NO_GEODE
# define GLOBED_ASSERT_LOG geode::log::error
#endif

#if GLOBED_CAN_USE_SOURCE_LOCATION && !defined(GLOBED_ROOT_NO_GEODE)
# define GLOBED_ASSERT(condition,message) \
    if (!(condition)) [[unlikely]] { \
        auto ev_msg = message; \
        auto loc = GLOBED_SOURCE; \
        geode::log::error("Assertion failed at {}: {}", fmt::format("{}:{} ({})", loc.file_name(), loc.line(), loc.function_name()), ev_msg); \
        throw std::runtime_error(std::string("Globed assertion failed: ") + ev_msg); \
    }
# define GLOBED_HARD_ASSERT(condition,message) \
    if (!(condition)) [[unlikely]] { \
        auto ev_msg = message; \
        auto loc = GLOBED_SOURCE; \
        geode::log::error("Assertion failed at {}: {}", fmt::format("{}:{} ({})", loc.file_name(), loc.line(), loc.function_name()), ev_msg); \
        GLOBED_SUICIDE; \
    }
#else
# define GLOBED_ASSERT(condition,message) \
    if (!(condition)) [[unlikely]] { \
        auto ev_msg = message; \
        GLOBED_ASSERT_LOG(std::string("Assertion failed: ") + ev_msg); \
        throw std::runtime_error(std::string("Globed assertion failed: ") + ev_msg); \
    }
# define GLOBED_HARD_ASSERT(condition,message) \
    if (!(condition)) [[unlikely]] { \
        auto ev_msg = message; \
        GLOBED_ASSERT_LOG(std::string("Assertion failed: ") + ev_msg); \
        GLOBED_SUICIDE; \
    }
#endif

#define GLOBED_UNIMPL(message) GLOBED_ASSERT(false, std::string("unimplemented: ") + (message))
