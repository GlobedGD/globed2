#pragma once
#include <defs.hpp>

#include <random>

#include <util/data.hpp>

namespace util::rng {
    class Random {
    public:
        GLOBED_SINGLETON(Random)
        Random();

        template <typename T>
        T generate();

        template <typename T>
        T generate(T max);

        template <typename T>
        T generate(T min, T max);

    private:
        std::mt19937_64 engine;
    };
}