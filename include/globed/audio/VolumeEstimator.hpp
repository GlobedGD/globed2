#pragma once

#include "AudioSampleQueue.hpp"
#include <asp/time/Instant.hpp>
#include <globed/prelude.hpp>

namespace globed {

class GLOBED_DLL VolumeEstimator {
public:
    VolumeEstimator(size_t sampleRate = 0);

    VolumeEstimator(const VolumeEstimator &) = default;
    VolumeEstimator &operator=(const VolumeEstimator &) = default;

    VolumeEstimator(VolumeEstimator &&) = default;
    VolumeEstimator &operator=(VolumeEstimator &&) = default;

    void feedData(const float *pcm, size_t samples);

    void update(float dt);

    float getVolume();

private:
    static constexpr float BUFFER_SIZE = 1.2f;

    float m_emaVolume = 0.f;
    float m_normalizedVolume = 0.f;
    size_t m_sampleRate = 0;
    AudioSampleQueue m_sampleQueue;
    std::deque<std::pair<asp::time::Instant, size_t>> m_writeHistory;
};

} // namespace globed
