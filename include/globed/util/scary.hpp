#pragma once

#include <Geode/Loader.hpp>
#include <type_traits>

namespace globed {
template <typename OutInput, typename In, typename Out = std::remove_pointer_t<OutInput>> Out *vtable_cast(In *obj)
{
    static_assert(std::is_polymorphic_v<Out> && std::is_pointer_v<OutInput>);

    static auto outVtable = [] {
        Out out;
        auto *vtable = *reinterpret_cast<void ***>(&out);
        return vtable;
    }();

    *reinterpret_cast<void ***>(obj) = outVtable;
    return reinterpret_cast<Out *>(obj);
}

void setObjectTypeName(cocos2d::CCNode *obj, std::string_view name);
} // namespace globed