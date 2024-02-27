#include <util/simd.hpp>
#include <util/misc.hpp>
#include <arm_neon.h>

static float pcmVolumeNEON(const float* pcm, size_t samples) {
#ifdef GEODE_IS_ANDROID64
    size_t alignedSamples = samples / 4 * 4;

    float32x4_t sumVec = vdupq_n_f32(0.0f);
    uint32x4_t maskVec = vdupq_n_u32(0x7fffffff);

    for (size_t i = 0; i < alignedSamples; i += 4) {
        float32x4_t pcmVec = vld1q_f32(pcm + i);
        pcmVec = vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(pcmVec), maskVec));
        sumVec = vaddq_f32(sumVec, pcmVec);
    }

    float32x2_t sumVecLow = vget_low_f32(sumVec);
    float32x2_t sumVecHigh = vget_high_f32(sumVec);
    float sum = vaddv_f32(sumVecLow) + vaddv_f32(sumVecHigh);

    for (size_t i = alignedSamples; i < samples; i++) {
        sum += std::abs(pcm[i]);
    }

    return sum / samples;
#else
    return util::misc::pcmVolumeSlow(pcm, samples);
#endif
}

float util::simd::calcPcmVolume(const float* pcm, size_t samples) {
    return pcmVolumeNEON(pcm, samples);
}
