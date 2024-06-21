#pragma once

#if defined(__arm__)

#include <cstddef>

namespace globed::simd::arm {
    float pcmVolume(const float* pcm, std::size_t samples);
}

#endif