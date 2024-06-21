#pragma once

#include <platform/basic.hpp>

#ifdef GLOBED_ARM

#include <cstddef>

namespace globed::simd::arm {
    float pcmVolume(const float* pcm, std::size_t samples);
}

#endif