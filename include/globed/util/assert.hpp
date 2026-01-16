#pragma once

#include <globed/config.hpp>
#include <string_view>

#define GLOBED_ASSERT(condition)                                                                                       \
    do {                                                                                                               \
        if (!(condition)) [[unlikely]] {                                                                               \
            ::globed::_assertionFail(#condition, __FILE__, __LINE__);                                                  \
        }                                                                                                              \
    } while (false)

#ifdef GLOBED_DEBUG
#define GLOBED_DEBUG_ASSERT(c) GLOBED_ASSERT(c)
#else
#define GLOBED_DEBUG_ASSERT(c) ((void)0)
#endif

namespace globed {
[[noreturn]] GLOBED_DLL void _assertionFail(std::string_view what, std::string_view file, int line);
}
