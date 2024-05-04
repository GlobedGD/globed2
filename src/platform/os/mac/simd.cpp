#include <util/simd.hpp>
#include <util/misc.hpp>

#include <platform/arch/x86/x86simd.hpp>

float util::simd::calcPcmVolume(const float *pcm, size_t samples) {
    return globed::simd::x86::pcmVolume(pcm, samples);
}
