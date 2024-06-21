#include <Geode/platform/cplatform.h>

#include <util/simd.hpp>
#include <util/misc.hpp>

#ifdef GEODE_IS_ARM_MAC
# include <platform/arch/arm/armsimd.hpp>
#else
# include <platform/arch/x86/x86simd.hpp>
#endif

float util::simd::calcPcmVolume(const float *pcm, size_t samples) {
#ifdef GEODE_IS_ARM_MAC
    return globed::simd::arm::pcmVolume(pcm, samples);
#else
    return globed::simd::x86::pcmVolume(pcm, samples);
#endif
}
