#pragma once
#include <defs.hpp>

namespace util::math {
    constexpr float FLOAT_ERROR_MARGIN = 0.002f;
    constexpr double DOUBLE_ERROR_MARGIN = 0.0001;

    // if `val` is NaN, return a signaling NaN, otherwise return `val` unchanged
    float snan(float val);
    // returns a signaling NaN
    float snan();

    // Returns `true` if all passed numbers are valid. Returns `false` if at least one of them is NaN
    template <typename... Args>
    requires (std::floating_point<Args> && ...)
    bool checkNotNaN(Args... args) {
        return ((!std::isnan(args)) && ...);
    }

    // `val1` == `val2`
    bool equal(float val1, float val2, float errorMargin = FLOAT_ERROR_MARGIN);
    // `val1` == `val2`
    bool equal(double val1, double val2, double errorMargin = DOUBLE_ERROR_MARGIN);

    // `val1` > `val2`
    bool greater(float val1, float val2, float errorMargin = FLOAT_ERROR_MARGIN);
    // `val1` > `val2`
    bool greater(double val1, double val2, double errorMargin = DOUBLE_ERROR_MARGIN);
    // `val1` >= `val2`
    bool greaterOrEqual(float val1, float val2, float errorMargin = FLOAT_ERROR_MARGIN);
    // `val1` >= `val2`
    bool greaterOrEqual(double val1, double val2, double errorMargin = DOUBLE_ERROR_MARGIN);

    // `val1` < `val2`
    bool smaller(float val1, float val2, float errorMargin = FLOAT_ERROR_MARGIN);
    // `val1` < `val2`
    bool smaller(double val1, double val2, double errorMargin = DOUBLE_ERROR_MARGIN);
    // `val1` <= `val2`
    bool smallerOrEqual(float val1, float val2, float errorMargin = FLOAT_ERROR_MARGIN);
    // `val1` <= `val2`
    bool smallerOrEqual(double val1, double val2, double errorMargin = DOUBLE_ERROR_MARGIN);
}