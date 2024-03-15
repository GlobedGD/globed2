#include "x86simd.hpp"

#include <defs/platform.hpp>
#include <defs/minimal_geode.hpp>
#include <util/misc.hpp>
#include <util/crypto.hpp>

namespace globed::simd::x86 {
    void cpuid(int info[4], int infoType) {
#ifdef GEODE_IS_WINDOWS
        __cpuid(info, infoType);
#else
        __asm__ __volatile__(
            "cpuid"
            : "=a" (info[0]), "=b" (info[1]), "=c" (info[2]), "=d" (info[3])
            : "a" (infoType)
        );
#endif
    }

    const CPUFeatures& getFeatures() {
        #define FEATURE(where, name, bit) features.name = (where & (1 << bit)) != 0

        static util::misc::OnceCell<CPUFeatures> cell;
        return cell.getOrInit([] {
            CPUFeatures features;
            int arr[4];
            cpuid(arr, 1);

            int eax = arr[0];
            int ebx = arr[1];
            int ecx = arr[2];
            int edx = arr[3];

            FEATURE(ecx, sse3, 0);
            FEATURE(ecx, pclmulqdq, 1);
            FEATURE(ecx, ssse3, 9);
            FEATURE(ecx, sse4_1, 19);
            FEATURE(ecx, sse4_2, 20);
            FEATURE(ecx, aes, 25);
            FEATURE(ecx, avx, 28);
            FEATURE(edx, sse, 25);
            FEATURE(edx, sse2, 26);

            // call again with eax = 7
            {
                int arr[4];
                arr[2] = 0;
                cpuid(arr, 7);

                int eax = arr[0];
                int ebx = arr[1];
                int ecx = arr[2];
                int edx = arr[3];

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

            // log::debug("Supported cpu features: {}", featureList);

            return features;
        });

        #undef FEATURE
    }

    float vec128sum(__m128 vec) {
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
        const auto& features = getFeatures();

        if (features.avx512dq) {
            return pcmVolumeAVX512(pcm, samples);
        } else if (features.avx2) {
            return pcmVolumeAVX2(pcm, samples);
        } else {
            return pcmVolumeSSE(pcm, samples);
        }
    }

    uint32_t adler32(const uint8_t* data, size_t length) {
        return util::crypto::adler32Slow(data, length);
        // const auto& features = getFeatures();

        // if (features.avx2) {
        //     return adler32AVX2(data, length);
        // } else if (features.sse2) {
        //     return adler32SSE2(data, length);
        // } else {
        //     return adler32Slow(data, length);
        // }
    }
}