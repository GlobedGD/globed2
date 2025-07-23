#pragma once
#include <stdint.h>

namespace globed {

template <typename T> requires std::floating_point<T>
inline T saneFmod(T x, std::type_identity_t<T> y) {
    T res = std::fmod(x, y);
    return res < 0 ? res + std::fabs(y) : res;
}

// Converts an arbitrary angle to a range of -180 to 180 degrees.
template <typename T> requires std::floating_point<T>
inline T normalizeAngle(T angle) {
    return static_cast<T>(saneFmod(static_cast<double>(angle) + 180.0, 360.0) - 180.0);
}

// Interpolates between two angles, choosing the shortest path.
// The resulting angle may not be normalized, for that use `lerpAngleNormalized`
template <typename T, typename Y> requires std::floating_point<T>
inline T lerpAngle(T a, T b, Y t) {
    T delta = normalizeAngle(b - a);
    return a + delta * t;
}

// Like `lerpAngle`, but normalizes the result to a range of -180 to 180 degrees.
template <typename T, typename Y> requires std::floating_point<T>
inline T lerpAngleNormalized(T a, T b, Y t) {
    return normalizeAngle(lerpAngle(a, b, t));
}

}
