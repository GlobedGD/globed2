#include "x86simd.hpp"
#include <cmath>

namespace globed::simd::x86 {
    float pcmVolumeSSE(const float* pcm, size_t samples) {
        size_t alignedSamples = samples / 4 * 4;

        __m128 sumVec = _mm_setzero_ps();
        __m128 maskVec = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));

        for (size_t i = 0; i < alignedSamples; i += 4) {
            __m128 pcmVec = _mm_loadu_ps(pcm + i);
            pcmVec = _mm_and_ps(pcmVec, maskVec);
            sumVec = _mm_add_ps(sumVec, pcmVec);
        }

        float sum = vec128sum(sumVec);

        for (size_t i = alignedSamples; i < samples; i++) {
            sum += std::abs(pcm[i]);
        }

        return sum / samples;
    }

    float GLOBED_FEATURE_AVX2 pcmVolumeAVX2(const float* pcm, size_t samples) {
        size_t alignedSamples = samples / 8 * 8;

        __m256 sumVec = _mm256_setzero_ps();
        __m256 maskVec = _mm256_castsi256_ps(_mm256_set1_epi32(0x7fffffff));

        for (size_t i = 0; i < alignedSamples; i += 8) {
            __m256 pcmVec = _mm256_loadu_ps(pcm + i);
            pcmVec = _mm256_and_ps(pcmVec, maskVec);
            sumVec = _mm256_add_ps(sumVec, pcmVec);
        }

        float sum = vec256sum(sumVec);

        for (size_t i = alignedSamples; i < samples; i++) {
            sum += std::abs(pcm[i]);
        }

        return sum / samples;
    }

    float GLOBED_FEATURE_AVX512DQ pcmVolumeAVX512(const float* pcm, size_t samples) {
        size_t alignedSamples = samples / 16 * 16;
        __m512 sumVec = _mm512_setzero_ps();
        __m512 maskVec = _mm512_castsi512_ps(_mm512_set1_epi32(0x7fffffff));

        for (size_t i = 0; i < alignedSamples; i += 16) {
            __m512 pcmVec = _mm512_loadu_ps(pcm + i);
            pcmVec = _mm512_and_ps(pcmVec, maskVec);
            sumVec = _mm512_add_ps(sumVec, pcmVec);
        }

        float sum = vec512sum(sumVec);

        for (size_t i = alignedSamples; i < samples; i++) {
            sum += std::abs(pcm[i]);
        }

        return sum / samples;
    }
}