#include <globed/audio/VolumeEstimator.hpp>
#include <asp/iter.hpp>

using namespace geode::prelude;

namespace globed {


VolumeEstimator::VolumeEstimator(size_t sampleRate) {
    m_sampleRate = sampleRate;
}

VolumeEstimator::VolumeEstimator() {
    m_sampleRate = 0;
}

void VolumeEstimator::feedData(const float* pcm, size_t samples) {
    m_sampleQueue.writeData(pcm, samples);

    if (m_sampleQueue.size() > static_cast<float>(m_sampleRate) * BUFFER_SIZE) {
        m_sampleQueue.clear();
    }
}

static float calculatePcmVolume(const float* pcm, size_t samples) {
    if (samples == 0) return 0.f;

    double sum = asp::iter::from(pcm, samples).map([](float v) { return v * v; }).sum();

    return static_cast<float>(sqrt(sum / (double)samples));
}

void VolumeEstimator::update(float dt) {
    if (std::isnan(dt)) {
        dt = 0.f;
    }

    dt = std::clamp(dt, 0.0f, 0.25f);

    const size_t needed = static_cast<size_t>(static_cast<float>(m_sampleRate) * BUFFER_SIZE * dt);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wvla-cxx-extension"
    float buf[needed];
#pragma clang diagnostic pop

    size_t copied = m_sampleQueue.readData(buf, needed);

    if (copied < needed) {
        // fill the rest with void
        for (size_t i = copied; i < needed; i++) {
            buf[i] = 0.f;
        }
    }

    float newVolume = calculatePcmVolume(buf, needed);
    m_emaVolume = qn::exponentialMovingAverage(m_emaVolume, newVolume, 0.2);
    m_normalizedVolume = std::powf(std::clamp(m_emaVolume / 0.25f, 0.f, 1.f), 0.5f);
}

float VolumeEstimator::getVolume() {
    return m_normalizedVolume;
}

}
