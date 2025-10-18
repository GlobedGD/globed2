#pragma once
#include <Geode/platform/platform.hpp>

namespace globed {

template <uintptr_t Version, uintptr_t Offset, bool Cocos = false>
struct PatchDef {
    template <auto = 0>
    operator uintptr_t() const {
        static_assert(Version == GEODE_COMP_GD_VERSION, "Patch must be updated to a new version!");
        return Offset;
    }

    template <typename Ty = void*>
    Ty addr() const {
        static_assert(Version == GEODE_COMP_GD_VERSION, "Patch must be updated to a new version!");
#ifdef GEODE_IS_WINDOWS
        if constexpr (Cocos) {
            return (Ty)(geode::base::getCocos() + Offset);
        } else
#endif
        {
            return (Ty)(geode::base::get() + Offset);
        }
    }
};

#ifdef GEODE_IS_WINDOWS

constexpr inline PatchDef<22074, 0x4932b0> PATCH_EGO_GETSAVESTRING_START;

#elif defined GEODE_IS_ANDROID32

#elif defined GEODE_IS_ANDROID64

#elif defined GEODE_IS_ARM_MAC

constexpr inline PatchDef<22074, 0xaa390> PATCH_SAVE_PERCENTAGE_CALL;

#elif defined GEODE_IS_INTEL_MAC

#elif defined GEODE_IS_IOS

#endif

#undef DefPatch

}
