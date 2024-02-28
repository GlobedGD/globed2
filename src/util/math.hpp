#pragma once
#include <cmath>
#include <limits>

namespace util::math {
    constexpr float FLOAT_ERROR_MARGIN = 0.002f;
    constexpr double DOUBLE_ERROR_MARGIN = 0.0001;

    // returns a signaling NaN
    inline float snan() {
        return std::numeric_limits<float>::signaling_NaN();
    }

    // if `val` is NaN, return a signaling NaN, otherwise return `val` unchanged
    inline float snan(float val) {
        return std::isnan(val) ? snan() : val;
    }

    // Returns `true` if all passed numbers are valid. Returns `false` if at least one of them is NaN
    template <typename... Args> requires (std::floating_point<Args> && ...)
    bool checkNotNaN(Args... args) {
        return ((!std::isnan(args)) && ...);
    }

    // floating point math is fun

    // `val1` == `val2`
    inline bool equal(float val1, float val2, float errorMargin = FLOAT_ERROR_MARGIN) {
        return std::fabs(val2 - val1) < errorMargin;
    }
    // `val1` == `val2`
    inline bool equal(double val1, double val2, double errorMargin = DOUBLE_ERROR_MARGIN) {
        return std::abs(val2 - val1) < errorMargin;
    }

    // `val1` > `val2`
    inline bool greater(float val1, float val2, float errorMargin = FLOAT_ERROR_MARGIN) {
        return val1 > val2 && std::fabs(val2 - val1) > errorMargin;
    }
    // `val1` > `val2`
    inline bool greater(double val1, double val2, double errorMargin = DOUBLE_ERROR_MARGIN) {
        return val1 > val2 && std::abs(val2 - val1) > errorMargin;
    }
    // `val1` >= `val2`
    inline bool greaterOrEqual(float val1, float val2, float errorMargin = FLOAT_ERROR_MARGIN) {
        return val1 > val2 || equal(val1, val2, errorMargin);
    }
    // `val1` >= `val2`
    inline bool greaterOrEqual(double val1, double val2, double errorMargin = DOUBLE_ERROR_MARGIN) {
        return val1 > val2 || equal(val1, val2, errorMargin);
    }

    // `val1` < `val2`
    inline bool smaller(float val1, float val2, float errorMargin = FLOAT_ERROR_MARGIN) {
        return val1 < val2 && std::fabs(val2 - val1) > errorMargin;
    }
    // `val1` < `val2`
    inline bool smaller(double val1, double val2, double errorMargin = DOUBLE_ERROR_MARGIN) {
        return val1 < val2 && std::abs(val2 - val1) > errorMargin;
    }
    // `val1` <= `val2`
    inline bool smallerOrEqual(float val1, float val2, float errorMargin = FLOAT_ERROR_MARGIN) {
        return val1 < val2 || equal(val1, val2, errorMargin);
    }
    // `val1` <= `val2`
    inline bool smallerOrEqual(double val1, double val2, double errorMargin = DOUBLE_ERROR_MARGIN) {
        return val1 < val2 || equal(val1, val2, errorMargin);
    }
}