#pragma once

#include "../util/assert.hpp"
#include <Geode/Result.hpp>
#include <unordered_map>

namespace globed {

/// The ABI version must be incremented when changes that aren't forwards or backwards compatible.
/// The following changes can be done without incrementing:
/// * Adding a new function in a subtable (functions are resolved dynamically, so it's ok)
/// * Removing functions in a subtable (will just return Err, no ABI break)
/// * Repurposing a reserved subtable, it's completely ok to rename the field, type and category name as long as position is kept.
/// The following changes require an ABI increment:
/// * Changing signature of an existing function
/// * Adding/removing/reordering subtables
static constexpr uint32_t GLOBED_ABI = 1;

static uint64_t fnv1aHash(std::string_view str) {
    uint64_t hash = 0xcbf29ce484222325;
    for (char c : str) {
        hash ^= c;
        hash *= 0x100000001b3;
    }
    return hash;
}

class FunctionTable {
public:
    inline void insert(std::string_view name, auto* func) {
        GLOBED_ASSERT(m_writable && "Cannot add functions to a finalized FunctionTable");
        m_functions[fnv1aHash(name)] = reinterpret_cast<void*>(func);
    }

    inline void finalize() {
        m_writable = false;
    }

    inline uint32_t abi() const {
        return m_abi;
    }

    template <typename FTy>
    inline geode::Result<FTy> getFunction(std::string_view name) {
        auto it = m_functions.find(fnv1aHash(name));
        if (it == m_functions.end()) {
            return geode::Err("Function not found in the table: '{}'", name);
        }

        auto func = reinterpret_cast<FTy>(it->second);
        return geode::Ok(func);
    }

    template <typename T, typename... Args>
    inline geode::Result<T> invoke(std::string_view name, Args... args) {
        using FTy = geode::Result<T>(*)(Args...);
        GEODE_UNWRAP_INTO(auto func, this->getFunction<FTy>(name));

        return func(std::move(args)...);
    }

private:
    uint32_t m_abi = GLOBED_ABI;
    std::unordered_map<uint64_t, void*> m_functions;
    bool m_writable = true;
};

template <typename Table>
struct FunctionTableSubcat {
    Table* table;
    std::string_view name;

    template <typename T, typename... Args>
    inline geode::Result<T> invoke(std::string_view name, Args... args) {
        char combined[128];
        auto res = fmt::format_to(combined, "{}.{}", this->name, name);

        std::string_view sv{combined, res.out};
        return table->template invoke<T, Args...>(sv, std::move(args)...);
    }

    template <typename FTy>
    inline geode::Result<FTy> getFunction(std::string_view name) {
        char combined[128];
        auto res = fmt::format_to(combined, "{}.{}", this->name, name);

        std::string_view sv{combined, res.out};
        return table->template getFunction<FTy>(sv);
    }
};

}