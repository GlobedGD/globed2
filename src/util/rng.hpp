#pragma once
#include <defs.hpp>

#include <random>

#include <util/data.hpp>

namespace util::rng {
    class Random : public SingletonBase<Random> {
    protected:
        friend class SingletonBase;
        Random();

    public:
        template <typename T>
        T generate();

        template <typename T>
        T generate(T max);

        template <typename T>
        T generate(T min, T max);

        // Has a `(numerator/denominator)` chance of returning true.
        bool genRatio(uint32_t numerator, uint32_t denominator);

        // Has a `ratio%` chance of returning true. 1.0f is 100%, 0.0f is 0%
        bool genRatio(float ratio);
        // Has a `ratio%` chance of returning true. 1.0 is 100%, 0.0 is 0%
        bool genRatio(double ratio);

    private:
        std::mt19937_64 engine;
    };
}