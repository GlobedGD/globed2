#pragma once

#include <globed/prelude.hpp>
#include "AudioSampleQueue.hpp"

namespace globed {

class VolumeEstimator {
public:
    VolumeEstimator(size_t sampleRate);
    VolumeEstimator();

    VolumeEstimator(const VolumeEstimator&) = default;
    VolumeEstimator& operator=(const VolumeEstimator&) = default;

    VolumeEstimator(VolumeEstimator&&) = default;
    VolumeEstimator& operator=(VolumeEstimator&&) = default;

    void feedData(const float* pcm, size_t samples);

    void update(float dt);

    float getVolume();

private:
    static constexpr float BUFFER_SIZE = 1.0f;

    float m_volume;
    size_t m_sampleRate;
    AudioSampleQueue m_sampleQueue;
};

}
