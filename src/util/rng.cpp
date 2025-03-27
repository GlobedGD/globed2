#include "rng.hpp"

#include <defs/assert.hpp>

#define RNG_DEF(type) \
    template type Random::generate<type>(); \
    template type Random::generate<type>(type); \
    template type Random::generate<type>(type, type)

namespace util::rng {
    Random::Random() {
        std::random_device rdev;
        engine.seed(rdev());
    }

    std::mt19937_64& Random::getEngine() {
        return engine;
    }

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

    // In a perfect world, long would be same as long long on unix, or int on msvc.
    // In practice, despite being the same size, they name different types.
    // Thank you C++

    // RNG_DEF(uint64_t);
    // RNG_DEF(int64_t);
    // RNG_DEF(uint32_t);
    // RNG_DEF(int32_t);
    // RNG_DEF(uint16_t);
    // RNG_DEF(int16_t);

    RNG_DEF(short);
    RNG_DEF(unsigned short);
    RNG_DEF(int);
    RNG_DEF(unsigned int);
    RNG_DEF(long);
    RNG_DEF(unsigned long);
    RNG_DEF(long long);
    RNG_DEF(unsigned long long);

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

    bool Random::genRatio(uint32_t numerator, uint32_t denominator) {
        if (denominator == 0) {
            throw std::runtime_error("attempt to call Random::genRatio with denominator set to 0");
        }

        double probability = static_cast<double>(numerator) / denominator;
        return this->genRatio(probability);
    }

    bool Random::genRatio(float ratio) {
        return this->generate<float>() < ratio;
    }

    bool Random::genRatio(double ratio) {
        return this->generate<double>() < ratio;
    }

    std::string Random::genString(std::string_view alphabet, size_t size) {
        std::string out(size, '\0');
        for (size_t i = 0; i < size; i++) {
            auto idx = this->generate<size_t>(0, alphabet.size() - 1);
            out[i] = alphabet[idx];
        }

        return out;
    }

    template <>
    void Random::fill<uint8_t>(uint8_t* ptr, size_t count) {
        while (count >= sizeof(size_t)) {
            size_t num = this->generate<size_t>();
            *reinterpret_cast<size_t*>(ptr) = num;

            ptr += sizeof(size_t);
            count -= sizeof(size_t);
        }

        // less than sizeof(size_t) bytes remain now
        while (count) {
            *ptr = static_cast<uint8_t>(this->generate<uint16_t>());
            ptr++;
            count--;
        }
    }
}
