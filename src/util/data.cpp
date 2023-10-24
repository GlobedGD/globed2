#include "data.hpp"

#ifdef _MSC_VER
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

    // XXX those 3 below are probably implementation defined, prefer not to use them
    int16_t byteswapI16(int16_t value) {
        return (value >> 8) | ((value & 0xFF) << 8);
    }

    int32_t byteswapI32(int32_t value) {
        return ((value >> 24) & 0xFF) | ((value >> 8) & 0xFF00) | ((value << 8) & 0xFF0000) | ((value << 24) & 0xFF000000);
    }

    int64_t byteswapI64(int64_t value) {
        return ((value >> 56) & 0xFF) | ((value >> 40) & 0xFF00) | ((value >> 24) & 0xFF0000) | ((value >> 8) & 0xFF000000) |
            ((value << 8) & 0xFF00000000LL) | ((value << 24) & 0xFF0000000000LL) | ((value << 40) & 0xFF000000000000LL) |
            ((value << 56) & 0xFF00000000000000LL);
    }

    // Yes, this is bad. It compiles down to a single x86 `bswap` instruction though so i don't care!
    float byteswapF32(float val) {
        float copy = val;
        unsigned char *bytes = reinterpret_cast<unsigned char*>(&copy);
        unsigned char temp;
        
        temp = bytes[0];
        bytes[0] = bytes[3];
        bytes[3] = temp;
        
        temp = bytes[1];
        bytes[1] = bytes[2];
        bytes[2] = temp;

        return copy;
    }

    double byteswapF64(double val) {
        double copy = val;

        unsigned char *bytes = reinterpret_cast<unsigned char*>(&copy);
        unsigned char temp;
        
        temp = bytes[0];
        bytes[0] = bytes[7];
        bytes[7] = temp;
        
        temp = bytes[1];
        bytes[1] = bytes[6];
        bytes[6] = temp;
        
        temp = bytes[2];
        bytes[2] = bytes[5];
        bytes[5] = temp;
        
        temp = bytes[3];
        bytes[3] = bytes[4];
        bytes[4] = temp;

        return copy;
    }
};

#undef BSWAP16
#undef BSWAP32
#undef BSWAP64