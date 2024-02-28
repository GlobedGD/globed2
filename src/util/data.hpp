#pragma once
#include <defs/platform.hpp>

#include <vector>
#include <array>

namespace util::data {
    using byte = uint8_t;
    using bytevector = std::vector<byte>;

    template <size_t Count>
    using bytearray = std::array<byte, Count>;


    template <typename T>
    concept IsPrimitive = std::is_same_v<T, uint16_t> || std::is_same_v<T, uint32_t> ||
                    std::is_same_v<T, uint64_t> || std::is_same_v<T, float> ||
                    std::is_same_v<T, double> || std::is_same_v<T, int16_t> ||
                    std::is_same_v<T, int32_t> || std::is_same_v<T, int64_t> ||
                    std::is_same_v<T, uint8_t> || std::is_same_v<T, int8_t>;

    // macos github actions runner has no std::bit_cast support
#ifdef __cpp_lib_bit_cast
    template <typename To, typename From>
    inline constexpr To bit_cast(From value) noexcept {
        return std::bit_cast<To, From>(value);
    }
#else
    template <typename To, typename From>
    std::enable_if_t<sizeof(To) == sizeof(From) && std::is_trivially_copyable_v<From> && std::is_trivially_copyable_v<To>, To>
    inline bit_cast(From value) noexcept {
        static_assert(std::is_trivially_constructible_v<To>, "destination template argument for bit_cast must be trivially constructible");

        To dest;
        std::memcpy(&dest, &value, sizeof(To));
        return dest;
    }
#endif

#ifndef __cpp_lib_byteswap
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
        static_assert(IsPrimitive<T>, "Unsupported type for byteswap");

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

#else // __cpp_lib_byteswap
    template <typename T>
    inline constexpr T byteswap(T val) {
        static_assert(IsPrimitive<T>, "Unsupported type for byteswap");

        if constexpr (std::is_same_v<T, float>) {
            return bit_cast<float>(std::byteswap(bit_cast<uint32_t>(val)));
        } else if constexpr (std::is_same_v<T, double>) {
            return bit_cast<double>(std::byteswap(bit_cast<uint64_t>(val)));
        } else {
            return std::byteswap(val);
        }
    }
#endif

    template <typename T>
    inline T maybeByteswap(T val) {
        if constexpr (GLOBED_LITTLE_ENDIAN) {
            val = byteswap(val);
        }

        return val;
    }

    // Converts the bit count into bytes required to fit it.
    // That means, 15 or 16 bits equals 2 bytes, but 17 bits equals 3 bytes.
    constexpr size_t bitsToBytes(size_t bits) {
        return (bits + 7) / 8;
    }
}