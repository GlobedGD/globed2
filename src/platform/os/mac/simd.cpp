#include <util/simd.hpp>
#include <util/misc.hpp>

float util::simd::calcPcmVolume(const float *pcm, size_t samples) {
    return util::misc::pcmVolumeSlow(pcm, samples);
}
