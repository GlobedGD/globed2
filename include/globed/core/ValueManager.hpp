#pragma once

#include <globed/util/singleton.hpp>
#include <globed/util/ConstexprString.hpp>
#include "Constants.hpp"

#include <matjson.hpp>

namespace globed {

class GLOBED_DLL ValueManager : public SingletonBase<ValueManager> {
public:
    template <typename T>
    std::optional<T> getValue(std::string_view key) {
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

private:
    friend class SingletonBase;
};

template <typename T>
std::optional<T> value(std::string_view key) {
    return ValueManager::get().getValue<T>(key);
}

template <typename T>
void setValue(std::string_view key, T value) {
    ValueManager::get().set(key, matjson::Value(value));
}

/// Sets a flag to true, returning the previous value (false if unset)
/// Flag should be formatted like core.flags.*
inline bool swapFlag(std::string_view key) {
    bool result = value<bool>(key).value_or(false);
    setValue(key, true);

    return result;
}

}