#pragma once
#include <config.hpp>
#include "util.hpp"

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

#if GLOBED_CAN_USE_SOURCE_LOCATION
# define GLOBED_REQUIRE(condition,message) \
    if (!(condition)) [[unlikely]] { \
        auto ev_msg = (message); \
        auto loc = GLOBED_SOURCE; \
        log::warn("Condition failed at {}: {}", fmt::format("{}:{} ({})", loc.file_name(), loc.line(), loc.function_name()), ev_msg); \
        throw(std::runtime_error(std::string(ev_msg))); \
    }
# define GLOBED_HARD_ASSERT(condition,message) \
    if (!(condition)) [[unlikely]] { \
        auto ev_msg = (message); \
        auto loc = GLOBED_SOURCE; \
        log::error("Condition failed at {}: {}", fmt::format("{}:{} ({})", loc.file_name(), loc.line(), loc.function_name()), ev_msg); \
        GLOBED_SUICIDE; \
    }
# define GLOBED_REQUIRE_SAFE(condition, message) \
    if (!(condition)) [[unlikely]] { \
        auto ev_msg = (message); \
        auto loc = GLOBED_SOURCE; \
        log::warn("Condition failed at {}: {}", fmt::format("{}:{} ({})", loc.file_name(), loc.line(), loc.function_name()), ev_msg); \
        return geode::Err(std::string(ev_msg)); \
    }
#else
# define GLOBED_REQUIRE(condition,message) \
    if (!(condition)) [[unlikely]] { \
        auto ev_msg = (message); \
        log::error("Condition failed: {}", ev_msg); \
        throw(std::runtime_error(std::string(ev_msg))); \
    }
# define GLOBED_HARD_ASSERT(condition,message) \
    if (!(condition)) [[unlikely]] { \
        auto ev_msg = (message); \
        log::error("Condition failed: {}", ev_msg); \
        GLOBED_SUICIDE; \
    }
# define GLOBED_REQUIRE_SAFE(condition, message) \
    if (!(condition)) [[unlikely]] { \
        auto ev_msg = (message); \
        log::warn("Condition failed: {}", ev_msg); \
        return geode::Err(std::string(ev_msg)); \
    }
#endif

#define GLOBED_UNIMPL(message) GLOBED_REQUIRE(false, std::string("unimplemented: ") + (message))
#define GLOBED_UNWRAP(value) \
    do { \
        auto __resv = (value); \
        if (__resv.isErr()) return geode::Err(std::move(__resv.unwrapErr())); \
    } while (0); \

#define GLOBED_UNWRAP_INTO(value, dest) \
    auto GEODE_CONCAT(_uval_, __LINE__) = (value); \
    if (GEODE_CONCAT(_uval_, __LINE__).isErr()) return geode::Err(std::move(GEODE_CONCAT(_uval_, __LINE__).unwrapErr())); \
    dest = GEODE_CONCAT(_uval_, __LINE__).unwrap();

#define GLOBED_RESULT_ERRC(value) \
    do { \
        auto __result = (value); \
        if (__result.isErr()) { \
            ErrorQueues::get().error(__result.unwrapErr()); \
        } \
    } while (false);

#define GLOBED_RESULT_WARNC(value) \
    do { \
        auto __result = (value); \
        if (__result.isErr()) { \
            ErrorQueues::get().warn(__result.unwrapErr()); \
        } \
    } while (false);

#define GLOBED_RESULT_DBGWARNC(value) \
    do { \
        auto __result = (value); \
        if (__result.isErr()) { \
            ErrorQueues::get().debugWarn(__result.unwrapErr()); \
        } \
    } while (false);