#pragma once
#include <defs/platform.hpp>

#include <vector>
#include <array>

#include <asp/misc/traits.hpp>
#include <asp/data/util.hpp>

namespace util::data {
    using byte = uint8_t;
    using bytevector = std::vector<byte>;

    template <size_t Count>
    using bytearray = std::array<byte, Count>;

    template <typename T>
    concept IsPrimitive = asp::data::is_primitive<T>;

    // macos github actions runner has no std::bit_cast support
#ifdef __cpp_lib_bit_cast
    template <typename To, typename From>
    inline constexpr To bit_cast(From value) noexcept {
        return std::bit_cast<To, From>(value);
    }
#else
    template <typename To, typename From>
    inline To bit_cast(From value) noexcept {
        return asp::data::bit_cast<To, From>(value);
    }
#endif

    template <typename T>
    inline constexpr T byteswap(T val) {
        return asp::data::byteswap(val);
    }

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