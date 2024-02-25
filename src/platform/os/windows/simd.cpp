#include <util/simd.hpp>
#include <util/misc.hpp>
#include <intrin.h>
#include <immintrin.h>

// everything here was done just for fun don't judge me too harshly :D

#if defined(__clang__) || defined(__GNUC__)
# define GLOBED_FEATURE_AVX __attribute__((__target__("avx")))
# define GLOBED_FEATURE_AVX2 __attribute__((__target__("avx2")))
# define GLOBED_FEATURE_AVX512 __attribute__((__target__("avx512f")))
# define GLOBED_FEATURE_AVX512DQ __attribute__((__target__("avx512dq")))
#else // __clang__
// on msvc there's no need to set these
# define GLOBED_FEATURE_AVX
# define GLOBED_FEATURE_AVX2
# define GLOBED_FEATURE_AVX512
# define GLOBED_FEATURE_AVX512DQ
#endif // __clang__

namespace util::simd {
    struct CPUFeatures {
        bool sse3, pclmulqdq, ssse3, sse4_1, sse4_2, aes, avx, sse, sse2, avx2, avx512, avx512dq;
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

        // call again with eax = 7
        {
            int arr[4];
            arr[2] = 0;
            __cpuid(arr, 7);

            int eax = arr[0];
            int ebx = arr[1];
            int ecx = arr[2];
            int edx = arr[3];

            log::debug("cpuid with 7, eax = {:X}, ebx = {:X}, ecx = {:X}, edx = {:X}", eax, ebx, ecx, edx);

            FEATURE(ebx, avx2, 5);
            FEATURE(ebx, avx512, 16);
            FEATURE(ebx, avx512dq, 17);
        }

        std::vector<std::string> featureList;
        features.sse3 ? featureList.push_back("sse3") : (void)0;
        features.pclmulqdq ? featureList.push_back("pclmulqdq") : (void)0;
        features.ssse3 ? featureList.push_back("ssse3") : (void)0;
        features.sse4_1 ? featureList.push_back("sse4.1") : (void)0;
        features.sse4_2 ? featureList.push_back("sse4.2") : (void)0;
        features.aes ? featureList.push_back("aes") : (void)0;
        features.avx ? featureList.push_back("avx") : (void)0;
        features.sse ? featureList.push_back("sse") : (void)0;
        features.sse2 ? featureList.push_back("sse2") : (void)0;
        features.avx2 ? featureList.push_back("avx2") : (void)0;
        features.avx512 ? featureList.push_back("avx512") : (void)0;
        features.avx512dq ? featureList.push_back("avx512dq") : (void)0;

        log::debug("Supported cpu features: {}", featureList);

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
    _mm_store_ss(&result, sum);
    return result;
#endif
}

static float GLOBED_FEATURE_AVX2 vec256sum(__m256 vec) {
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

static float GLOBED_FEATURE_AVX512 vec512sum(__m512 vec) {
    return _mm512_reduce_add_ps(vec);
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

static float GLOBED_FEATURE_AVX2 pcmVolumeAVX2(const float* pcm, size_t samples) {
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

static float GLOBED_FEATURE_AVX512DQ pcmVolumeAVX512(const float* pcm, size_t samples) {
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

float util::simd::calcPcmVolume(const float *pcm, size_t samples) {
    auto& features = getFeatures();

    if (features.avx512dq) {
        return pcmVolumeAVX512(pcm, samples);
    } else if (features.avx2) {
        return pcmVolumeAVX2(pcm, samples);
    } else {
        return pcmVolumeSSE(pcm, samples);
    }
}