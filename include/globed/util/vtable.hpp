#pragma once
#include <Geode/Result.hpp>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace globed {

struct VTable {
    template <typename Self, typename T>
    bool hasFunction(this Self& self, T** ptr) {
        size_t offset = (size_t)ptr - (size_t)&self;
        return offset < self.m_size && *ptr != nullptr;
    }

    template <typename Self, typename T, typename... Args>
    std::invoke_result_t<T, Args...> safeInvoke(this Self& self, T** ptr, Args&&... args) {
        using Output = std::invoke_result_t<T, Args...>;

        if (self.hasFunction(ptr)) {
            return (*ptr)(std::forward<Args>(args)...);
        } else {
            if constexpr (geode::IsResult<Output>) {
                return geode::Err("Attempted to invoke a vtable function that does not exist");
            } else if constexpr (std::is_void_v<Output>) {
                return;
            } else {
                return Output{};
            }
        }
    }

protected:
    size_t m_size;

    VTable(size_t size) : m_size(size) {}
    VTable(const VTable&) = delete;
    VTable(VTable&&) = delete;
    VTable& operator=(const VTable&) = delete;
    VTable& operator=(VTable&&) = delete;
};

#define GLOBED_VTABLE_INNER_FN(...) GEODE_INVOKE(GEODE_CONCAT(GLOBED_VTABLE_INNER_FN_, GEODE_NUMBER_OF_ARGS(__VA_ARGS__)), __VA_ARGS__)
#define GLOBED_VTABLE_INNER_FN_2(ret, name) \
    inline auto name() { return this->safeInvoke(&_func_##name); }
#define GLOBED_VTABLE_INNER_FN_3(ret, name, p1) \
    inline auto name(p1 a1) { return this->safeInvoke(&_func_##name, std::move(a1)); }
#define GLOBED_VTABLE_INNER_FN_4(ret, name, p1, p2) \
    inline auto name(p1 a1, p2 a2) { return this->safeInvoke(&_func_##name, std::move(a1), std::move(a2)); }
#define GLOBED_VTABLE_INNER_FN_5(ret, name, p1, p2, p3) \
    inline auto name(p1 a1, p2 a2, p3 a3) { return this->safeInvoke(&_func_##name, std::move(a1), std::move(a2), std::move(a3)); }
#define GLOBED_VTABLE_INNER_FN_6(ret, name, p1, p2, p3, p4) \
    inline auto name(p1 a1, p2 a2, p3 a3, p4 a4) { return this->safeInvoke(&_func_##name, std::move(a1), std::move(a2), std::move(a3), std::move(a4)); }

#define GLOBED_VTABLE_FUNC(name, ret, ...) \
    ret (* _func_##name) (__VA_ARGS__); \
    GLOBED_VTABLE_INNER_FN(ret, name, __VA_ARGS__)

#define GLOBED_VTABLE_INIT(tptr, name, ...) \
    (tptr)->_func_##name = +[]__VA_ARGS__;

}
