#pragma once

#include "../util/assert.hpp"
#include <Geode/Result.hpp>
#include <unordered_map>

namespace globed {

static constexpr uint32_t GLOBED_ABI = 1;

class FunctionTable {
public:
    inline void insert(std::string_view name, auto* func) {
        GLOBED_ASSERT(m_writable && "Cannot add functions to a finalized FunctionTable");
        m_functions[name] = reinterpret_cast<void*>(func);
    }

    inline void finalize() {
        m_writable = false;
    }

    inline uint32_t abi() const {
        return m_abi;
    }

    template <typename T, typename... Args>
    inline geode::Result<T> invoke(std::string_view name, Args&&... args) {
        using FTy = geode::Result<T>(*)(Args...);

        auto it = m_functions.find(name);
        if (it == m_functions.end()) {
            return geode::Err("Function not found in the table: '{}'", name);
        }

        auto func = reinterpret_cast<FTy>(it->second);
        auto result = func(std::forward<Args>(args)...);

        return result;
    }

private:
    uint32_t m_abi = GLOBED_ABI;
    std::unordered_map<std::string_view, void*> m_functions;
    bool m_writable = true;
};

}