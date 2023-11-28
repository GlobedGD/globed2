#pragma once
#include <defs.hpp>

namespace util::math {
    constexpr float FLOAT_ERROR_MARGIN = 0.002f;
    constexpr double DOUBLE_ERROR_MARGIN = 0.0001;

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