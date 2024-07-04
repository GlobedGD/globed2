#pragma once
#include <type_traits>

// into

namespace globed {

// Utility function for converting various types between each other, is specialized separately for different types
template <typename To, typename From>
    requires (!std::is_same_v<To, From>)
To into(const From& value);

// Specialization for T -> T
template <typename T>
inline T into(const T& value) {
    return value;
}

}