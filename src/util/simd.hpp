#pragma once
#include <defs.hpp>

namespace util::simd {
    struct CPUFeatures;

    const CPUFeatures& getFeatures();
    float calcPcmVolume(const float* pcm, size_t samples);
}