#include "lowlevel.hpp"

namespace util::lowlevel {
    void nop(ptrdiff_t offset, size_t bytes) {
#ifdef GEODE_IS_WINDOWS
        std::vector<uint8_t> bytevec;
        for (size_t i = 0; i < bytes; i++) {
            bytevec.push_back(0x90);
        }

        (void) Mod::get()->patch(reinterpret_cast<void*>(geode::base::get() + offset), bytevec);
#else
        throw std::runtime_error("nop not implemented");
#endif
    }
}