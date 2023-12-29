#pragma once
#include <defs.hpp>

namespace util::misc {
    // Utility function for converting various enums between each other, is specialized separately for different enums
    template <typename To, typename From>
    To convertEnum(From value);
}