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
#ifndef __cpp_lib_byteswap
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
        return bit_cast<int16_t>(byteswapU16(bit_cast<uint16_t>(value)));
    }

    int32_t byteswapI32(int32_t value) {
        return bit_cast<int32_t>(byteswapU32(bit_cast<uint32_t>(value)));
    }

    int64_t byteswapI64(int64_t value) {
        return bit_cast<int64_t>(byteswapU64(bit_cast<uint64_t>(value)));
    }

    float byteswapF32(float value) {
        return bit_cast<float>(byteswapU32(bit_cast<uint32_t>(value)));
    }

    double byteswapF64(double value) {
        return bit_cast<double>(byteswapU64(bit_cast<uint64_t>(value)));
    }
#endif // __cpp_lib_byteswap
};

#undef BSWAP16
#undef BSWAP32
#undef BSWAP64