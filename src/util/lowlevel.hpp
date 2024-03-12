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

    // Cast a polymorphic object to another (potentially unrelated by inheritance) polymorphic object.
    // Advantages: significantly faster than dynamic_cast or typeinfo_cast (simply compares main vtable pointer)
    // Disadvantages:
    // * will only work if `object` is an exact instance of `To`, will not work if `To` is a parent class
    // * will only work if `To` has a bound constructor
    template <typename To, typename From>
    requires std::is_polymorphic_v<To> && std::is_polymorphic_v<From>
    To* vtable_cast(From* object) {
        if (!object) return nullptr;

        To obj;
        return vtable_cast_with_obj(object, &obj);
    }

    // Like `vtable_cast` but lifts the requirement of `To` having a default constructor.
    // You must provide an instance of `To`
    template <typename To, typename From>
    requires std::is_polymorphic_v<To> && std::is_polymorphic_v<From>
    To* vtable_cast_with_obj(From* object, To* toInst) {
        if (!object || !toInst) return nullptr;

        void** vtableFrom = *reinterpret_cast<void***>(object);
        void** vtableTo = *reinterpret_cast<void***>(toInst);

        if (vtableFrom == vtableTo) {
            return reinterpret_cast<To*>(object);
        }

        return nullptr;
    }
}