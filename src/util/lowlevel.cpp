#include "lowlevel.hpp"
#include <defs/assert.hpp>

#include <util/net.hpp>

namespace util::lowlevel {
    geode::Patch* patch(ptrdiff_t offset, const std::vector<uint8_t>& bytes) {
        return patchAbsolute(geode::base::get() + offset, bytes);
    }

    geode::Patch* patchAbsolute(uintptr_t address, const std::vector<uint8_t>& bytes) {
        auto p = Mod::get()->patch(reinterpret_cast<void*>(address), bytes);

        if (!p) {
            log::error("Failed to apply patch at {:X}: {}", address, p.unwrapErr());
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

#elif defined(GEODE_IS_ARM_MAC) || defined(GEODE_IS_ANDROID64)
        GLOBED_REQUIRE(bytes % 4 == 0, "patch must be a multiple of 4 on ARM64");

        std::vector<uint8_t> nopvec;
        for (size_t i = 0; i < bytes; i += 4) {
            nopvec.push_back(0x1f);
            nopvec.push_back(0x20);
            nopvec.push_back(0x03);
            nopvec.push_back(0xd5);
        }
        return patch(offset, nopvec);
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

    Result<> withProtectedMemory(void* address, size_t size, std::function<void(void*)>&& callback) {
#ifdef GEODE_IS_WINDOWS
        DWORD oldProtect;

        if (VirtualProtect(address, size, PAGE_READWRITE, &oldProtect)) {
            callback(address);
            VirtualProtect(address, size, oldProtect, &oldProtect);
        } else {
            return Err(util::net::lastErrorString(GetLastError(), false));
        }

        return Ok();
#endif
        return Err("withProtectedMemory unimplemented");
    }
}