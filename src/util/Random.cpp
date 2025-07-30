#include <globed/util/Random.hpp>
#include <random>

using namespace geode::prelude;

namespace globed {

bool Rng::randomRatio(double ratio) {
    ratio = std::clamp(ratio, 0.0, 1.0);
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(m_engine) < ratio;
}

void Rng::fillBytes(void* buffer, size_t size) {
    uint8_t* buf = static_cast<uint8_t*>(buffer);

    while (size > 8) {
        uint64_t value = m_engine();
        std::memcpy(buf, &value, sizeof(value));
        buf += sizeof(value);
        size -= sizeof(value);
    }

    if (size > 0) {
        uint64_t value = m_engine();
        std::memcpy(buf, &value, size);
    }
}

ThreadRngHandle::ThreadRngHandle(std::shared_ptr<Rng> rng) : m_rng(std::move(rng)) {}

ThreadRngHandle::ThreadRngHandle() {}

ThreadRngHandle::~ThreadRngHandle() {}

ThreadRngHandle rng() {
    static thread_local ThreadRngHandle handle{};

    if (!handle) {
        handle = ThreadRngHandle{std::make_shared<Rng>()};
    }

    return handle;
}

}
