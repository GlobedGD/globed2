#pragma once

#include <Geode/utils/general.hpp>
#include <defs/minimal_geode.hpp>

namespace util::lowlevel {
    geode::Patch* patch(ptrdiff_t offset, const std::vector<uint8_t>& bytes);

    // nop X bytes at offset
    geode::Patch* nop(ptrdiff_t offset, size_t bytes);

    // replace the instruction at `offset` with a relative call
    geode::Patch* relativeCall(ptrdiff_t offset, ptrdiff_t callableOffset);
    geode::Patch* absoluteCall(ptrdiff_t offset, ptrdiff_t callableOffset);

    // picks either relativeCall or absoluteCall depending on the circumstance.
    geode::Patch* call(ptrdiff_t offset, ptrdiff_t callableOffset);

    geode::Result<> withProtectedMemory(void* address, size_t size, std::function<void(void*)>&& callback);

    template <typename Func>
    concept FPtr = std::is_function_v<Func>;

    template <typename DFunc>
    Result<Patch*> vmtHookWithTable(DFunc detour, void** vtable, size_t index) {
        auto bytes = geode::toByteArray(detour);
        bytes.resize(sizeof(void*));

        auto result = Mod::get()->patch(&vtable[index], bytes);

        if (result) {
            return result;
        }

        auto err = result.unwrapErr();
        if (err.find("Unable to write to memory") != std::string::npos) {
            Result<Patch*> out;

            auto protResult = withProtectedMemory(&vtable[index], bytes.size(), [&](void* addr) {
                out = Mod::get()->patch(addr, bytes);
            });

            if (!protResult) {
                return Err(protResult.unwrapErr());
            }

            return out;
        }

        return result;
    }

    // perform a VMT hook. Note that on windows, this will only be able to hook functions in the main vtable.
    template <typename DFunc, typename ObjT>
    Result<Patch*> vmtHook(DFunc detour, ObjT* object, size_t index) {
        void** vtable = *reinterpret_cast<void***>(object);
        return vmtHookWithTable(detour, vtable, index);
    }

    // Assuming that `Out` inherits `In`, downcasts `In*` to `Out*`, and replaces the vtable with that of `Out`.
    // This is not safe. Do not do this.
    template <typename Out, typename In>
        requires (std::is_base_of_v<In, Out> && std::is_default_constructible_v<Out> && std::is_polymorphic_v<In>)
    Out* swapVtable(In* ptr) {
        static_assert(sizeof(In) == sizeof(Out), "swapVtable is not safe to use if the sizes of the classes do not match");
        Out tempInstance;

        std::memcpy((void*)ptr, (void*)&tempInstance, sizeof(void*));

        return reinterpret_cast<Out*>(ptr);
    }

    // Cast a polymorphic object to another (potentially unrelated by inheritance) polymorphic object.
    // Advantages: significantly faster than dynamic_cast or typeinfo_cast (simply compares main vtable pointer)
    // Disadvantages:
    // * will only work if `object` is an exact instance of `To`, will not work if `To` is a parent class
    // * will only work if `To` has a bound constructor
    template <typename To, typename From>
    requires std::is_pointer_v<To> && std::is_polymorphic_v<std::remove_pointer_t<To>> && std::is_polymorphic_v<From>
    To vtable_cast(From* object) {
        if (!object) return nullptr;
        using Inst = typename std::remove_pointer_t<To>;

        Inst obj;
        return vtable_cast_with_obj(object, &obj);
    }

    // Like `vtable_cast` but lifts the requirement of `To` having a default constructor.
    // You must provide an instance of `To`
    template <typename To, typename From>
    requires std::is_pointer_v<To> && std::is_polymorphic_v<std::remove_pointer_t<To>> && std::is_polymorphic_v<From>
    To vtable_cast_with_obj(From* object, To toInst) {
        if (!object || !toInst) return nullptr;

        void** vtableTo = *reinterpret_cast<void***>(toInst);

        return vtable_cast_with_table<To>(object, vtableTo);
    }

    // Like `vtable_cast` but you provide the vtable pointer yourself.
    template <typename To, typename From>
    requires std::is_pointer_v<To> && std::is_polymorphic_v<std::remove_pointer_t<To>> && std::is_polymorphic_v<From>
    To vtable_cast_with_table(From* object, void* vtableTo) {
        if (!object || !vtableTo) return nullptr;

        void** vtableFrom = *reinterpret_cast<void***>(object);

        if (vtableFrom == vtableTo) {
            return reinterpret_cast<To*>(object);
        }

        return nullptr;
    }
}