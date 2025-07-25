#pragma once

#include <globed/util/singleton.hpp>

namespace globed {

class ValueManager : public SingletonBase<ValueManager> {
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

}