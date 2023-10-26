#pragma once

#include <cstdint>
#include <vector>

namespace util::data {
    using byte = uint8_t;
    using bytevector = std::vector<byte>;

    uint16_t byteswapU16(uint16_t val);
    uint32_t byteswapU32(uint32_t val);
    uint64_t byteswapU64(uint64_t val);

    int16_t byteswapI16(int16_t val);
    int32_t byteswapI32(int32_t val);
    int64_t byteswapI64(int64_t val);

    float byteswapF32(float val);
    double byteswapF64(double val);

    template <typename T>
    inline T byteswap(T val) {
        // I am so sorry
        static_assert(std::is_same_v<T, uint16_t> || std::is_same_v<T, uint32_t> ||
                  std::is_same_v<T, uint64_t> || std::is_same_v<T, float> ||
                  std::is_same_v<T, double> || std::is_same_v<T, int16_t> ||
                  std::is_same_v<T, int32_t> || std::is_same_v<T, int64_t> ||
                  std::is_same_v<T, uint8_t> || std::is_same_v<T, int8_t>,
                  "Unsupported type for byteswap");

        if constexpr (std::is_same_v<T, uint16_t>) {
            return byteswapU16(val);
        } else if constexpr (std::is_same_v<T, uint32_t>) {
            return byteswapU32(val);
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            return byteswapU64(val);
        } else if constexpr (std::is_same_v<T, float>) {
            return byteswapF32(val);
        } else if constexpr (std::is_same_v<T, double>) {
            return byteswapF64(val);
        } else if constexpr (std::is_same_v<T, int16_t>) {
            return byteswapI16(val);
        } else if constexpr (std::is_same_v<T, int32_t>) {
            return byteswapI32(val);
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return byteswapI64(val);
        } else if constexpr (std::is_same_v<T, uint8_t> || std::is_same_v<T, int8_t>) {
            return val;
        }
    }

    // Converts the bit count into bytes required to fit it.
    // That means, 15 or 16 bits equals 2 bytes, but 17 bits equals 3 bytes.
    constexpr size_t bitsToBytes(size_t bits) {
        return (bits + 7) / 8;
    }
};