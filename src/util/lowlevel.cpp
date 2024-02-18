#include "lowlevel.hpp"

#ifdef GEODE_IS_WINDOWS
# include <immintrin.h>
#endif

namespace util::lowlevel {
    void nop(ptrdiff_t offset, size_t bytes) {
#ifdef GEODE_IS_WINDOWS
        std::vector<uint8_t> bytevec;
        for (size_t i = 0; i < bytes; i++) {
            bytevec.push_back(0x90);
        }

        (void) Mod::get()->patch(reinterpret_cast<void*>(geode::base::get() + offset), bytevec);
#else
        throw std::runtime_error("nop not implemented");
#endif
    }

    // i only did this as an excuse to learn simd :D
    float pcmVolumeFast(const float* pcm, size_t samples) {
#if defined(GEODE_IS_WINDOWS) && defined(__clang__)
        size_t alignedSamples = samples / 4 * 4;

        __m128 sumVec = _mm_setzero_ps();
        __m128 maskVec = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));

        for (size_t i = 0; i < alignedSamples; i += 4) {
            __m128 pcmVec = _mm_loadu_ps(pcm + i);
            pcmVec = _mm_and_ps(pcmVec, maskVec);
            sumVec = _mm_add_ps(sumVec, pcmVec);
        }

        float sum = sumVec[0] + sumVec[1] + sumVec[2] + sumVec[3];

        for (size_t i = alignedSamples; i < samples; i++) {
            sum += std::abs(pcm[i]);
        }

        return sum / samples;
#else
        return pcmVolumeSlow(pcm, samples);
#endif
    }

    float pcmVolumeSlow(const float* pcm, size_t samples) {
        double sum = 0.0f;
        for (size_t i = 0; i < samples; i++) {
            sum += static_cast<double>(std::abs(pcm[i]));
        }

        return static_cast<float>(sum / static_cast<double>(samples));
    }
}