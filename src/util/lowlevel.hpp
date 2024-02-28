#pragma once

#include <Geode/utils/general.hpp>
#include <defs/minimal_geode.hpp>

namespace util::lowlevel {
    // nop X bytes at offset
    geode::Patch* nop(ptrdiff_t offset, size_t bytes);

    template <typename Func>
    concept FPtr = std::is_function_v<Func>;

    template <typename DFunc>
    void vmtHookWithTable(DFunc detour, void** vtable, size_t index) {
        auto bytes = geode::toByteArray(detour);
        bytes.resize(sizeof(void*));

#ifdef GEODE_IS_WINDOWS
        (void) Mod::get()->patch(&vtable[index], bytes);
#else
        (void) Mod::get()->patch(&vtable[index], bytes);
#endif
    }

    // perform a VMT hook. Note that on windows, this will only be able to hook functions in the main vtable.
    template <typename DFunc, typename ObjT>
    void vmtHook(DFunc detour, ObjT* object, size_t index) {
        void** vtable = *reinterpret_cast<void***>(object);
        vmtHookWithTable(detour, vtable, index);
    }
}