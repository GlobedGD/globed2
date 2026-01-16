#pragma once

#include "Constants.hpp"
#include <globed/util/ConstexprString.hpp>
#include <globed/util/singleton.hpp>

#include <matjson.hpp>
#include <std23/function_ref.h>

namespace globed {

class GLOBED_DLL ValueManager : public SingletonBase<ValueManager> {
public:
    template <typename T> std::optional<T> getValue(std::string_view key)
    {
        auto raw = this->getValueRaw(key);

        if (raw) {
            if (auto val = raw->as<T>()) {
                return *val;
            } else {
                return std::nullopt;
            }
        } else {
            return std::nullopt;
        }
    }

    std::optional<matjson::Value> getValueRaw(std::string_view key);

    void set(std::string_view key, matjson::Value value);
    void erase(std::string_view key);
    void eraseWithPrefix(std::string_view prefix);
    void eraseMatching(std23::function_ref<bool(std::string_view)> predicate);

private:
    friend class SingletonBase;
};

template <typename T> std::optional<T> value(std::string_view key)
{
    return ValueManager::get().getValue<T>(key);
}

template <typename T> void setValue(std::string_view key, T value)
{
    ValueManager::get().set(key, matjson::Value(value));
}

inline bool flag(std::string_view key)
{
    return value<bool>(key).value_or(false);
}

/// Sets a flag to true, returning the previous value (false if unset)
/// Flag should be formatted like core.flags.*
inline bool swapFlag(std::string_view key)
{
    bool result = flag(key);
    setValue(key, true);

    return result;
}

} // namespace globed