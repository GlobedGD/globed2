#pragma once

#include <platform/basic.hpp>
#include <asp/simd.hpp>

#ifdef GLOBED_X86

#include <immintrin.h>
#include <cstddef>

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
    using CPUFeatures = asp::simd::CPUFeatures;

    /* Generic functions */

    float GLOBED_FEATURE_AVX2 vec256sum(__m256 vec);
    float GLOBED_FEATURE_AVX512 vec512sum(__m512 vec);


    /* Functions that auto pick the fastest algorithm */


    // Calculate the volume of pcm samples, picking the fastest possible implementation.
    float pcmVolume(const float* pcm, size_t samples);


    /* Functions written with a specific algorithm */


    float pcmVolumeSSE(const float* pcm, size_t samples);
    float GLOBED_FEATURE_AVX2 pcmVolumeAVX2(const float* pcm, size_t samples);
    float GLOBED_FEATURE_AVX512DQ pcmVolumeAVX512(const float* pcm, size_t samples);
}

#endif