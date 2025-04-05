#include <Geode/platform/cplatform.h>

#include <util/simd.hpp>
#include <util/misc.hpp>

#include <platform/arch/arm/armsimd.hpp>

float util::simd::calcPcmVolume(const float* pcm, size_t samples) {
    return globed::simd::arm::pcmVolume(pcm, samples);
}
