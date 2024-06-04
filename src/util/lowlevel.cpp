#include "lowlevel.hpp"
#include <defs/assert.hpp>

namespace util::lowlevel {
    geode::Patch* patch(ptrdiff_t offset, const std::vector<uint8_t>& bytes) {
        auto p = Mod::get()->patch(reinterpret_cast<void*>(geode::base::get() + offset), bytes);

        if (!p) {
            log::error("Failed to apply patch at {:X}: {}", offset, p.unwrapErr());
            return nullptr;
        }

        return p.unwrap();
    }

    geode::Patch* nop(ptrdiff_t offset, size_t bytes) {
#if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_MACOS)
        std::vector<uint8_t> bytevec;
        for (size_t i = 0; i < bytes; i++) {
            bytevec.push_back(0x90);
        }

        return patch(offset, bytevec);
#else
        GLOBED_UNIMPL("util::lowlevel::nop");
#endif
    }

    geode::Patch* relativeCall(ptrdiff_t offset, ptrdiff_t callableOffset) {
#if defined(GLOBED_IS_X86_64)
        std::vector<uint8_t> bytes;
        bytes.push_back(0xe8); // rip-relative CALL
        bytes.resize(5);

        uint32_t relAddress = static_cast<uint32_t>(callableOffset - offset - 5 /* size of a relative CALL instruction */);

        std::memcpy(bytes.data() + 1, &relAddress, sizeof(uintptr_t));

        return patch(offset, bytes);
#elif defined(GLOBED_IS_X86)

        GLOBED_UNIMPL("util::lowlevel::relativeCall");
#else
        GLOBED_UNIMPL("util::lowlevel::relativeCall");
#endif
    }

    geode::Patch* absoluteCall(ptrdiff_t offset, ptrdiff_t callableOffset) {
        GLOBED_UNIMPL("util::lowlevel::absoluteCall");
    }

    geode::Patch* call(ptrdiff_t offset, ptrdiff_t callableOffset) {
        if (callableOffset > offset) {
            return relativeCall(offset, callableOffset);
        } else {
            return absoluteCall(offset, callableOffset);
        }
    }
}