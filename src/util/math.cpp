#include "math.hpp"

namespace util::math {
    float snan(float val) {
        return std::isnan(val) ? snan() : val;
    }

    float snan() {
        return std::numeric_limits<float>::signaling_NaN();
    }

    bool equal(float val1, float val2, float errorMargin) {
        return std::fabs(val2 - val1) < errorMargin;
    }

    bool equal(double val1, double val2, double errorMargin) {
        return std::abs(val2 - val1) < errorMargin;
    }

    bool greater(float val1, float val2, float errorMargin) {
        return val1 > val2 && std::fabs(val2 - val1) > errorMargin;
    }

    bool greater(double val1, double val2, double errorMargin) {
        return val1 > val2 && std::abs(val2 - val1) > errorMargin;
    }

    bool greaterOrEqual(float val1, float val2, float errorMargin) {
        return val1 > val2 || equal(val1, val2, errorMargin);
    }

    bool greaterOrEqual(double val1, double val2, double errorMargin) {
        return val1 > val2 || equal(val1, val2, errorMargin);
    }

    bool smaller(float val1, float val2, float errorMargin) {
        return val1 < val2 && std::fabs(val2 - val1) > errorMargin;
    }

    bool smaller(double val1, double val2, double errorMargin) {
        return val1 < val2 && std::abs(val2 - val1) > errorMargin;
    }

    bool smallerOrEqual(float val1, float val2, float errorMargin) {
        return val1 < val2 || equal(val1, val2, errorMargin);
    }

    bool smallerOrEqual(double val1, double val2, double errorMargin) {
        return val1 < val2 || equal(val1, val2, errorMargin);
    }
}