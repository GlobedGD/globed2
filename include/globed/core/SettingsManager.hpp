#pragma once

#include <globed/util/singleton.hpp>
#include "ValueManager.hpp"

namespace globed {

template <typename T>
class SettingAccessor {
public:
    SettingAccessor(std::string_view key);

    operator T() const;
    SettingAccessor& operator=(T value);

private:
    uint64_t hash;
};

template <typename T>
class NoHashHasher;

template <>
class NoHashHasher<uint64_t> {
public:
    size_t operator()(uint64_t key) const {
        return key;
    }
};

class SettingsManager : public SingletonBase<SettingsManager> {
public:
    template <typename T>
    SettingAccessor<T> setting(std::string_view key) {
        return SettingAccessor<T>(key);
    }

    template <typename T>
    T getSettingRaw(uint64_t hash) {
        if (!this->hasSetting(hash)) {
            // internal error, means we used a wrong id somewhere
            throw std::runtime_error(fmt::format("setting not found with hash {}", hash));
        }

        auto set = m_settings.at(hash);
        if (auto res = set.as<T>()) {
            return *res;
        } else {
            // type mismatch, user may have edited the config? who knows, just return the default
            return m_defaults[hash];
        }
    }

    template <typename T>
    void setSettingRaw(uint64_t hash, T value) {
        if (!this->hasSetting(hash)) {
            // internal error, means we used a wrong id somewhere
            throw std::runtime_error(fmt::format("setting not found with hash {}", hash));
        }

        m_settings[hash] = value;
        ValueManager::get().set(m_fullKeys[hash], value);
    }

    bool hasSetting(uint64_t hash);

private:
    friend class SingletonBase;
    template <typename T>
    friend class SettingAccessor;

    bool m_frozen = false;
    std::unordered_map<uint64_t, matjson::Value, NoHashHasher<uint64_t>> m_settings;
    std::unordered_map<uint64_t, matjson::Value, NoHashHasher<uint64_t>> m_defaults;
    std::unordered_map<uint64_t, std::string, NoHashHasher<uint64_t>> m_fullKeys;

    SettingsManager();

    void add(
        std::string_view key,
        std::string_view name,
        std::string_view description,
        matjson::Value defaultVal
        // TODO: limits
    );

    void freeze();

    // fnv-1a hash with no extra operations
    static uint64_t finalKeyHash(std::string_view key);
    // faster shorthand for `finalKeyHash(fmt::format("setting.{}", key))`
    static uint64_t keyHash(std::string_view key);
};

template <typename T>
SettingAccessor<T> setting(std::string_view key) {
    return SettingsManager::get().setting<T>(key);
}

template <typename T>
SettingAccessor<T>::SettingAccessor(std::string_view key) : hash(SettingsManager::keyHash(key)) {}

template <typename T>
SettingAccessor<T>::operator T() const {
    return SettingsManager::get().getSettingRaw<T>(hash);
}

template <typename T>
SettingAccessor<T>& SettingAccessor<T>::operator=(T value) {
    SettingsManager::get().setSettingRaw<T>(hash, std::move(value));
    return *this;
}

}