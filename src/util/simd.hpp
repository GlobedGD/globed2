#pragma once
#include <stdint.h>
#include <stddef.h>

namespace util::simd {
    float calcPcmVolume(const float* pcm, size_t samples);

    uint32_t adler32(const uint8_t* data, size_t len);
}