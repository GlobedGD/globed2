#pragma once
#include <defs.hpp>
#include <random>

#include <util/data.hpp>

namespace util::rng {
    // generate `size` bytes of cryptographically secure random data
    data::bytevector secureRandom(size_t size);
    // generate `size` bytes of cryptographically secure random data into the given buffer
    void secureRandom(data::byte* dest, size_t size);

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