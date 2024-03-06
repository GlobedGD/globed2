#pragma once
#include <immintrin.h>

// everything here was done just for fun and educational purposes don't judge me too harshly :D

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

namespace globed::simd::x86 {
    struct CPUFeatures {
        bool sse3, pclmulqdq, ssse3, sse4_1, sse4_2, aes, avx, sse, sse2, avx2, avx512, avx512dq;
    };

    /* Generic functions */

    void cpuid(int info[4], int infoType);

    const CPUFeatures& getFeatures();

    float vec128sum(__m128 vec);
    float GLOBED_FEATURE_AVX2 vec256sum(__m256 vec);
    float GLOBED_FEATURE_AVX512 vec512sum(__m512 vec);


    /* Functions that auto pick the fastest algorithm */


    // Calculate the volume of pcm samples, picking the fastest possible implementation.
    float pcmVolume(const float* pcm, size_t samples);

    // Calculate the adler32 checksum of the given data, picking the fastest possible implementation
    uint32_t adler32(const uint8_t* data, size_t length);


    /* Functions written with a specific algorithm */


    float pcmVolumeSSE(const float* pcm, size_t samples);
    float GLOBED_FEATURE_AVX2 pcmVolumeAVX2(const float* pcm, size_t samples);
    float GLOBED_FEATURE_AVX512DQ pcmVolumeAVX512(const float* pcm, size_t samples);

    // TODO
    // uint32_t adler32SSE2(const uint8_t* data, size_t length);
    // uint32_t GLOBED_FEATURE_AVX2 adler32AVX2(const uint8_t* data, size_t length);
}