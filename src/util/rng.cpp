#include "rng.hpp"
#include <util/crypto.hpp>

#define RNG_DEF(type) \
    template type Random::generate<type>(); \
    template type Random::generate<type>(type); \
    template type Random::generate<type>(type, type)

namespace util::rng {
    data::bytevector secureRandom(size_t size) {
        data::bytevector out(size);
        secureRandom(out.data(), size);
        return out;
    }

    void secureRandom(data::byte* dest, size_t size) {
        CRYPTO_SODIUM_INIT
        randombytes_buf(dest, size);
    }

    Random::Random() {
        std::random_device rdev;
        engine.seed(rdev());
    }

    GLOBED_SINGLETON_DEF(Random)

    template <typename T>
    T Random::generate() {
        return generate(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
    }

    template <typename T>
    T Random::generate(T max) {
        return generate(std::numeric_limits<T>::min(), max);
    }

    template <typename T>
    T Random::generate(T min, T max) {
        std::uniform_int_distribution<T> dist(min, max);
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

    // floating point methods

    // Generates a float with arbitrary limits
    template<> float Random::generate<float>(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(engine);
    }

    // Generates a double with arbitrary limits
    template<> double Random::generate<double>(double min, double max) {
        std::uniform_real_distribution<double> dist(min, max);
        return dist(engine);
    }

    // Generates a float between 0.0 and 1.0
    template<> float Random::generate<float>() {
        return generate(0.f, 1.f);
    }

    // Generates a double between 0.0 and 1.0
    template<> double Random::generate<double>() {
        return generate(0.0, 1.0);
    }
}