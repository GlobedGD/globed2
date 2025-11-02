#pragma once

#include <type_traits>
#include <Geode/Loader.hpp>

namespace globed {
    template <typename OutInput, typename In, typename Out = std::remove_pointer_t<OutInput>>
    Out* vtable_cast(In* obj) {
        static_assert(std::is_polymorphic_v<Out> && std::is_pointer_v<OutInput>);

        static auto outVtable = [] {
            Out out;
            auto* vtable = *reinterpret_cast<void***>(&out);
            return vtable;
        }();

        *reinterpret_cast<void***>(obj) = outVtable;
        return reinterpret_cast<Out*>(obj);
    }

    template <typename T>
    void megaQueue(T&& func, size_t frames) {
        if (frames == 0) {
            func();
            return;
        }

        geode::Loader::get()->queueInMainThread([func = std::forward<T>(func), frames]() {
            megaQueue(func, frames - 1);
        });
    }

    void setObjectTypeName(cocos2d::CCNode* obj, std::string_view name);
}