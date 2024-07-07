#pragma once
#include <cmath>
#include <limits>
#include <concepts>

namespace util::math {
    constexpr float FLOAT_ERROR_MARGIN = 0.002f;
    constexpr double DOUBLE_ERROR_MARGIN = 0.0001;

    // returns a signaling NaN
    inline constexpr float snan() {
        return std::numeric_limits<float>::signaling_NaN();
    }

    // if `val` is NaN, return a signaling NaN, otherwise return `val` unchanged
    inline constexpr float snan(float val) {
        return std::isnan(val) ? snan() : val;
    }

    template <typename T>
    inline constexpr T abs(T val) {
        return (val < 0) ? -val : val;
    }

    // Returns `true` if all passed numbers are valid. Returns `false` if at least one of them is NaN
    template <typename... Args> requires (std::floating_point<Args> && ...)
    bool constexpr checkNotNaN(Args... args) {
        return ((!std::isnan(args)) && ...);
    }

    // floating point math is fun

    // `val1` == `val2`
    inline constexpr bool equal(float val1, float val2, float errorMargin = FLOAT_ERROR_MARGIN) {
        return abs(val2 - val1) < errorMargin;
    }
    // `val1` == `val2`
    inline constexpr bool equal(double val1, double val2, double errorMargin = DOUBLE_ERROR_MARGIN) {
        return abs(val2 - val1) < errorMargin;
    }

    // `val1` > `val2`
    inline constexpr bool greater(float val1, float val2, float errorMargin = FLOAT_ERROR_MARGIN) {
        return val1 > val2 && abs(val2 - val1) > errorMargin;
    }
    // `val1` > `val2`
    inline constexpr bool greater(double val1, double val2, double errorMargin = DOUBLE_ERROR_MARGIN) {
        return val1 > val2 && abs(val2 - val1) > errorMargin;
    }
    // `val1` >= `val2`
    inline constexpr bool greaterOrEqual(float val1, float val2, float errorMargin = FLOAT_ERROR_MARGIN) {
        return val1 > val2 || equal(val1, val2, errorMargin);
    }
    // `val1` >= `val2`
    inline constexpr bool greaterOrEqual(double val1, double val2, double errorMargin = DOUBLE_ERROR_MARGIN) {
        return val1 > val2 || equal(val1, val2, errorMargin);
    }

    // `val1` < `val2`
    inline constexpr bool smaller(float val1, float val2, float errorMargin = FLOAT_ERROR_MARGIN) {
        return val1 < val2 && fabs(val2 - val1) > errorMargin;
    }
    // `val1` < `val2`
    inline constexpr bool smaller(double val1, double val2, double errorMargin = DOUBLE_ERROR_MARGIN) {
        return val1 < val2 && abs(val2 - val1) > errorMargin;
    }
    // `val1` <= `val2`
    inline constexpr bool smallerOrEqual(float val1, float val2, float errorMargin = FLOAT_ERROR_MARGIN) {
        return val1 < val2 || equal(val1, val2, errorMargin);
    }
    // `val1` <= `val2`
    inline constexpr bool smallerOrEqual(double val1, double val2, double errorMargin = DOUBLE_ERROR_MARGIN) {
        return val1 < val2 || equal(val1, val2, errorMargin);
    }

    template <typename T1, typename T2, typename Out = std::common_type_t<T1, T2>>
    inline constexpr Out (min)(T1 a, T2 b) {
        return a < b ? a : b;
    }

    template <typename T1, typename T2, typename Out = std::common_type_t<T1, T2>>
    inline constexpr Out (max)(T1 a, T2 b) {
        return a > b ? a : b;
    }
}