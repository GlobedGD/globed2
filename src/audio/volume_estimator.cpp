#include "volume_estimator.hpp"

#include "manager.hpp"
#include <util/misc.hpp>

#if GLOBED_VOICE_SUPPORT

VolumeEstimator::VolumeEstimator(size_t sampleRate) {
    this->sampleRate = sampleRate;
}

VolumeEstimator::VolumeEstimator() {
    this->sampleRate = 0;
}

void VolumeEstimator::feedData(const float* pcm, size_t samples) {
    sampleQueue.writeData(pcm, samples);

    if (sampleQueue.size() > static_cast<float>(sampleRate) * BUFFER_SIZE) {
        sampleQueue.clear();
    }
}

void VolumeEstimator::update(float dt) {
    if (std::isnan(dt)) {
        dt = 0.f;
    }

    dt = std::clamp(dt, 0.0f, 0.25f);

    const size_t needed = static_cast<size_t>(static_cast<float>(sampleRate) * BUFFER_SIZE * dt);

    // TODO, this is a bit delayed when buffer size is 1.0, and overall broken when it's below that

#ifdef __clang__
    float buf[needed];
#else
    // yuck msvc
    float* buf = reinterpret_cast<float*>(alloca(sizeof(float) * needed));
#endif
    size_t copied = sampleQueue.copyTo(buf, needed);

    if (copied < needed) {
        // fill the rest with void
        for (size_t i = copied; i < needed; i++) {
            buf[i] = 0.f;
        }
    }

    volume = util::misc::calculatePcmVolume(buf, needed);
}

float VolumeEstimator::getVolume() {
    return volume;
}


#endif // GLOBED_VOICE_SUPPORT