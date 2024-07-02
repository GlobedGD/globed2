#include "x86simd.hpp"

#ifdef GLOBED_X86

#include <defs/platform.hpp>
#include <defs/minimal_geode.hpp>
#include <util/misc.hpp>
#include <util/crypto.hpp>

namespace globed::simd::x86 {
    float GLOBED_FEATURE_AVX2 vec256sum(__m256 vec) {
        // https://copyprogramming.com/howto/how-to-sum-m256-horizontally
        const __m128 hiQuad = _mm256_extractf128_ps(vec, 1);
        const __m128 loQuad = _mm256_castps256_ps128(vec);
        const __m128 sumQuad = _mm_add_ps(loQuad, hiQuad);
        const __m128 loDual = sumQuad;
        const __m128 hiDual = _mm_movehl_ps(sumQuad, sumQuad);
        const __m128 sumDual = _mm_add_ps(loDual, hiDual);
        const __m128 lo = sumDual;
        const __m128 hi = _mm_shuffle_ps(sumDual, sumDual, 0x1);
        const __m128 sum = _mm_add_ps(lo, hi);

        return _mm_cvtss_f32(sum);
    }

    float GLOBED_FEATURE_AVX512 vec512sum(__m512 vec) {
        return _mm512_reduce_add_ps(vec);
    }

    float pcmVolume(const float* pcm, size_t samples) {
        const auto& features = asp::simd::getFeatures();

        if (features.avx512dq) {
            return pcmVolumeAVX512(pcm, samples);
        } else if (features.avx2) {
            return pcmVolumeAVX2(pcm, samples);
        } else {
            return pcmVolumeSSE(pcm, samples);
        }
    }
}

#endif