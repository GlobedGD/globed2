#include <util/simd.hpp>
#include <util/misc.hpp>

#include <platform/arch/x86/x86simd.hpp>

float util::simd::calcPcmVolume(const float *pcm, size_t samples) {
    return globed::simd::x86::pcmVolume(pcm, samples);
}

uint32_t util::simd::adler32(const uint8_t* data, size_t len) {
    return globed::simd::x86::adler32(data, len);
}
