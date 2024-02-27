#include "lowlevel.hpp"

namespace util::lowlevel {
    geode::Patch* nop(ptrdiff_t offset, size_t bytes) {
#if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_MACOS)
        std::vector<uint8_t> bytevec;
        for (size_t i = 0; i < bytes; i++) {
            bytevec.push_back(0x90);
        }

        return Mod::get()->patch(reinterpret_cast<void*>(geode::base::get() + offset), bytevec).unwrap();
#else
        throw std::runtime_error("nop not implemented");
#endif
    }
}