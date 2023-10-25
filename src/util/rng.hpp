#pragma once
#include <defs.hpp>
#include <random>

namespace util::rng {
    class Random {
    public:
        GLOBED_SINGLETON(Random)
        Random();

        template <typename T>
        T generate();

    private:
        std::mt19937_64 engine;
    };
}