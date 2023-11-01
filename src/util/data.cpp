#include "data.hpp"

#ifdef _MSC_VER
# include <stdlib.h>
# define BSWAP16(val) _byteswap_ushort(val)
# define BSWAP32(val) _byteswap_ulong(val)
# define BSWAP64(val) _byteswap_uint64(val)
#else
# define BSWAP16(val) ((val >> 8) | (val << 8))
# define BSWAP32(val) __builtin_bswap32(val)
# define BSWAP64(val) __builtin_bswap64(val)
#endif

namespace util::data {
    uint16_t byteswapU16(uint16_t val) {
        return BSWAP16(val);
    }

    uint32_t byteswapU32(uint32_t val) {
        return BSWAP32(val);
    }

    uint64_t byteswapU64(uint64_t val) {
        return BSWAP64(val);
    }

    int16_t byteswapI16(int16_t value) {
        return ((value & 0xFF) << 8) | ((value & 0xFF00) >> 8);
    }

    int32_t byteswapI32(int32_t value) {
        return ((value >> 24) & 0xFF) | ((value >> 8) & 0xFF00) | ((value << 8) & 0xFF0000) | ((value << 24) & 0xFF000000);
    }

    int64_t byteswapI64(int64_t value) {
        return ((value >> 56) & 0xFF) | ((value >> 40) & 0xFF00) | ((value >> 24) & 0xFF0000) | ((value >> 8) & 0xFF000000) |
            ((value << 8) & 0xFF00000000LL) | ((value << 24) & 0xFF0000000000LL) | ((value << 40) & 0xFF000000000000LL) |
            ((value << 56) & 0xFF00000000000000LL);
    }

    float byteswapF32(float val) {
        uint32_t num = std::bit_cast<uint32_t>(val);
        return std::bit_cast<float>(BSWAP32(num));
    }

    double byteswapF64(double val) {
        uint64_t num = std::bit_cast<uint64_t>(val);
        return std::bit_cast<double>(BSWAP64(num));
    }
};

#undef BSWAP16
#undef BSWAP32
#undef BSWAP64