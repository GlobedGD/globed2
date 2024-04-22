#pragma once

#include <stdint.h>

namespace globed::simd::arm {
    float pcmVolume(const float* pcm, size_t samples);
}