#pragma once
#include <memory>
#include <random>

namespace globed {

class Rng {
public:
    Rng() : m_engine(std::random_device{}()) {}

    std::mt19937_64& engine() {
        return m_engine;
    }

    template <std::integral T>
    T random() {
        T out;
        this->fill(&out, 1);
        return out;
    }

    template <>
    bool random() {
        return this->random<uint8_t>() % 2 == 0;
    }

    template <std::integral T>
    T random(T min, T max) {
        std::uniform_int_distribution<T> dist(min, max);
        return dist(m_engine);
    }

    template <std::floating_point T>
    T random(T min, T max) {
        std::uniform_real_distribution<T> dist(min, max);
        return dist(m_engine);
    }

    bool randomRatio(double ratio);
    void fillBytes(void* buffer, size_t size);

    template <typename T>
    void fill(T* buffer, size_t count) {
        static_assert(alignof(T) <= sizeof(T));

        return this->fillBytes(buffer, count * sizeof(T));
    }

    template <typename T>
    void fill(std::span<T> buffer) {
        return this->fill(buffer.data(), buffer.size());
    }

    template <typename T>
    void fill(std::vector<T>& buffer) {
        return this->fill(buffer.data(), buffer.size());
    }

private:
    std::mt19937_64 m_engine;
};

class ThreadRngHandle {
public:
    ThreadRngHandle(std::shared_ptr<Rng> rng);
    ThreadRngHandle();
    ~ThreadRngHandle();

    inline operator bool() const {
        return m_rng != nullptr;
    }

    inline Rng& operator*() const {
        return *m_rng;
    }

    inline Rng* operator->() const {
        return m_rng.get();
    }

protected:
    std::shared_ptr<Rng> m_rng;
};

ThreadRngHandle rng();

}
