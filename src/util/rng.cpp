#include "rng.hpp"

#define RNG_DEF(type) template type Random::generate<type>()

namespace util::rng {
    GLOBED_SINGLETON_GET(Random)

    Random::Random() {
        std::random_device rdev;
        engine.seed(rdev());
    }

    template <typename T>
    T Random::generate() {
        std::uniform_int_distribution<T> dist(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
        return dist(engine);
    }

    RNG_DEF(uint64_t);
    RNG_DEF(int64_t);
    RNG_DEF(uint32_t);
    RNG_DEF(int32_t);
    RNG_DEF(uint16_t);
    RNG_DEF(int16_t);

    template<> bool Random::generate<bool>() {
        std::uniform_int_distribution<int> dist(0, 1);
        return static_cast<bool>(dist(engine));
    }
}