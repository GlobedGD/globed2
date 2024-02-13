#include "volume_estimator.hpp"

#include "manager.hpp"
#include <util/misc.hpp>

#if GLOBED_VOICE_SUPPORT

VolumeEstimator::VolumeEstimator(size_t sampleRate) {
    this->sampleRate = sampleRate;
}

void VolumeEstimator::feedData(const float* pcm, size_t samples) {
    sampleQueue.writeData(pcm, samples);

    if (sampleQueue.size() > 24000) {
        sampleQueue.clear();
    }
}

float VolumeEstimator::getVolume() {
    constexpr size_t needed = VOICE_TARGET_SAMPLERATE / 60;

    float buf[needed];
    size_t copied = sampleQueue.copyTo(buf, needed);

    if (copied < needed) {
        // fill the rest with void
        for (size_t i = copied; i < needed; i++) {
            buf[i] = 0.f;
        }
    }

    return util::misc::calculatePcmVolume(buf, needed);
}


#endif // GLOBED_VOICE_SUPPORT