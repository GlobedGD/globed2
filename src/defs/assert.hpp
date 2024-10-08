#pragma once
#include <config.hpp>
#include "util.hpp"

#if defined(__cpp_lib_source_location) && __cpp_lib_source_location >= 201907L
# include <source_location>
#endif

#ifdef GLOBED_DEBUG
# include <boost/stacktrace/stacktrace.hpp>
#endif

/*
* GLOBED_REQUIRE - throws a runtime error if condition fails
* GLOBED_REQUIRE_SAFE - returns a Result with an Err value if condition fails
* GLOBED_HARD_ASSERT - terminates the program if condition fails. Don't use it unless the condition indicates a hard logic error in the code.
* GLOBED_UNIMPL - throws a runtime error as the method was not implemented and isn't meant to be called
* GLOBED_UNWRAP - return Err if a result is Err, otherwise do nothing
* GLOBED_UNWRAP_INTO - return Err if a result is Err, otherwise put the value into the destination
* GLOBED_RESULT_ERRC - call ErrorQueues::error() if result is Err, discard the value
* GLOBED_RESULT_WARNC - call ErrorQueues::warn() if result is Err, discard the value
* GLOBED_RESULT_DBGWARNC - call ErrorQueues::debugWarn() if result is Err, discard the value
*/

#ifdef GLOBED_DEBUG
# define GLOBED_REQUIRE(condition, message) \
    if (!(condition)) [[unlikely]] { \
        ::globed::_condFail(std::source_location::current(), ::boost::stacktrace::stacktrace{}, (message)); \
    }
# define GLOBED_HARD_ASSERT(condition, message) \
    if (!(condition)) [[unlikely]] { \
        ::globed::_condFailFatal(std::source_location::current(), ::boost::stacktrace::stacktrace{}, (message)); \
    }
# define GLOBED_REQUIRE_SAFE(condition, message) \
    if (!(condition)) [[unlikely]] { \
        return geode::Err(::globed::_condFailSafe(std::source_location::current(), ::boost::stacktrace::stacktrace{}, (message))); \
    }
#else
# define GLOBED_REQUIRE(condition,message) \
    if (!(condition)) [[unlikely]] { \
        ::globed::_condFail(std::source_location::current(), (message)); \
    }
# define GLOBED_HARD_ASSERT(condition,message) \
    if (!(condition)) [[unlikely]] { \
        ::globed::_condFailFatal(std::source_location::current(), (message)); \
    }
# define GLOBED_REQUIRE_SAFE(condition, message) \
    if (!(condition)) [[unlikely]] { \
        return geode::Err(::globed::_condFailSafe(std::source_location::current(), (message))); \
    }
#endif

// #else
// # define GLOBED_REQUIRE(condition,message) \
//     if (!(condition)) [[unlikely]] { \
//         auto ev_msg = (message); \
//         geode::log::error("Condition failed: {}", ev_msg); \
//         throw(std::runtime_error(std::string(ev_msg))); \
//     }
// # define GLOBED_HARD_ASSERT(condition,message) \
//     if (!(condition)) [[unlikely]] { \
//         auto ev_msg = (message); \
//         geode::log::error("Condition failed: {}", ev_msg); \
//         GLOBED_SUICIDE; \
//     }
// # define GLOBED_REQUIRE_SAFE(condition, message) \
//     if (!(condition)) [[unlikely]] { \
//         auto ev_msg = (message); \
//         geode::log::warn("Condition failed: {}", ev_msg); \
//         return geode::Err(std::string(ev_msg)); \
//     }
// #endif

#define GLOBED_UNIMPL(message) GLOBED_REQUIRE(false, std::string("unimplemented: ") + (message))
#define GLOBED_UNWRAP(value) \
    do { \
        auto __resv = (value); \
        if (__resv.isErr()) return geode::Err(std::move(__resv.unwrapErr())); \
    } while (0); \

#define GLOBED_UNWRAP_INTO(value, dest) \
    auto GEODE_CONCAT(_uval_, __LINE__) = std::move((value)); \
    if (GEODE_CONCAT(_uval_, __LINE__).isErr()) return geode::Err(std::move(GEODE_CONCAT(_uval_, __LINE__).unwrapErr())); \
    dest = std::move(GEODE_CONCAT(_uval_, __LINE__).unwrap());

#define GLOBED_RESULT_ERRC(value) \
    do { \
        auto __result = std::move((value)); \
        if (__result.isErr()) { \
            ErrorQueues::get().error(__result.unwrapErr()); \
        } \
    } while (false);

#define GLOBED_RESULT_WARNC(value) \
    do { \
        auto __result = std::move((value)); \
        if (__result.isErr()) { \
            ErrorQueues::get().warn(__result.unwrapErr()); \
        } \
    } while (false);

#define GLOBED_RESULT_DBGWARNC(value) \
    do { \
        auto __result = std::move((value)); \
        if (__result.isErr()) { \
            ErrorQueues::get().debugWarn(__result.unwrapErr()); \
        } \
    } while (false);

namespace globed {
    [[noreturn]] static inline void unreachable() {
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
        __assume(false);
#else // GCC, Clang
        __builtin_unreachable();
#endif
    }

    [[noreturn]] void _condFail(const std::source_location& loc, const boost::stacktrace::stacktrace& trace, std::string_view message);
    std::string _condFailSafe(const std::source_location& loc, const boost::stacktrace::stacktrace& trace, std::string_view message);
    [[noreturn]] void _condFailFatal(const std::source_location& loc, const boost::stacktrace::stacktrace& trace, std::string_view message);
}
