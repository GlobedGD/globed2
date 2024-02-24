#include <util/simd.hpp>
#include <util/misc.hpp>
#include <intrin.h>
#include <immintrin.h>

// everything here was done just for fun don't judge me too harshly :D

namespace util::simd {
    struct CPUFeatures {
        bool sse3, pclmulqdq, ssse3, sse4_1, sse4_2, aes, avx, sse, sse2, avx2;
    };
}

#define FEATURE(where, name, bit) features.name = (where & (1 << bit)) != 0
const util::simd::CPUFeatures& util::simd::getFeatures() {
    static misc::OnceCell<CPUFeatures> cell;
    return cell.getOrInit([] {
        CPUFeatures features;
        int arr[4];
        __cpuid(arr, 1);

        int eax = arr[0];
        int ebx = arr[1];
        int ecx = arr[2];
        int edx = arr[3];

        log::debug("cpuid eax = {:X}, ebx = {:X}, ecx = {:X}, edx = {:X}", eax, ebx, ecx, edx);

        FEATURE(ecx, sse3, 0);
        FEATURE(ecx, pclmulqdq, 1);
        FEATURE(ecx, ssse3, 9);
        FEATURE(ecx, sse4_1, 19);
        FEATURE(ecx, aes, 25);
        FEATURE(ecx, avx, 28);
        FEATURE(edx, sse, 25);
        FEATURE(edx, sse2, 26);
        FEATURE(ebx, avx2, 5);
        return features;
    });
}
#undef FEATURE

static float vec128sum(__m128 vec) {
#ifdef __clang__
    return vec[0] + vec[1] + vec[2] + vec[3];
#else
    __m128 sum = _mm_hadd_ps(vec, vec);
    sum = _mm_hadd_ps(sum, sum);

    float result;
    _mm_store_ss(&reslut, sum);
    return result;
#endif
}

static float pcmVolumeSSE(const float* pcm, size_t samples) {
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

float util::simd::calcPcmVolume(const float *pcm, size_t samples) {
    auto& features = getFeatures();

    return pcmVolumeSSE(pcm, samples);
}