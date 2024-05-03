#pragma once
#include <stdint.h>

namespace util::crypto {
    // scalar adler32 implementation
    constexpr inline uint32_t adler32Slow(const uint8_t* data, size_t length) {
        const uint32_t MOD = 65521;

        uint32_t a = 1, b = 0;

        for (size_t i = 0; i < length; i++) {
            a = (a + data[i]) % MOD;
            b = (b + a) % MOD;
        }

        return (b << 16) | a;
    }

    template <size_t N>
    constexpr inline uint32_t adler32Const(const char(&data)[N]) {
        const uint32_t MOD = 65521;

        uint32_t a = 1, b = 0;

        // make sure to exclude the null terminator
        for (size_t i = 0; i < N - 1; i++) {
            a = (a + data[i]) % MOD;
            b = (b + a) % MOD;
        }

        return (b << 16) | a;
    }
}