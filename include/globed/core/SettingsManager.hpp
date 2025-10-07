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
    using Validator = std::function<bool(const matjson::Value&)>;

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
            auto it = m_validators.find(hash);
            if (it == m_validators.end() || it->second(set)) {
                return *res;
            }
        }

        // type mismatch or validation failed, return default
        if (auto res = m_defaults[hash].as<T>()) {
            return *res;
        } else {
            // no default either, this is a serious error
            throw std::runtime_error(fmt::format("setting with hash {} has no default value or the default value is of an invalid type", hash));
        }
    }

    template <typename T>
    void setSettingRaw(uint64_t hash, T value) {
        if (!this->hasSetting(hash)) {
            // internal error, means we used a wrong id somewhere
            throw std::runtime_error(fmt::format("setting not found with hash {}", hash));
        }

        matjson::Value val = value;
        auto it = m_validators.find(hash);
        if (it != m_validators.end()) {
            if (!it->second(val)) {
                geode::log::warn("Failed to save setting {}, validation failed: {}", m_fullKeys[hash], val.dump(matjson::NO_INDENTATION));
                return;
            }
        }

        m_settings[hash] = val;
        ValueManager::get().set(m_fullKeys[hash], std::move(val));
    }

    std::optional<std::pair<matjson::Value, matjson::Value>> getLimits(std::string_view key);

    bool hasSetting(uint64_t hash);

    /// Registers a new setting. This cannot be called after the SettingsManager is frozen,
    /// which happens at the end of the loading phase when the core is initialized.
    void registerSetting(
        std::string_view key,
        matjson::Value defaultVal
        // TODO: limits
    );

    void registerValidator(
        std::string_view key,
        Validator func
    );

    void registerLimits(
        std::string_view key,
        matjson::Value min,
        matjson::Value max
    );

private:
    friend class SingletonBase;
    friend class CoreImpl;
    template <typename T>
    friend class SettingAccessor;

    bool m_frozen = false;
    std::unordered_map<uint64_t, matjson::Value, NoHashHasher<uint64_t>> m_settings;
    std::unordered_map<uint64_t, matjson::Value, NoHashHasher<uint64_t>> m_defaults;
    std::unordered_map<uint64_t, std::string, NoHashHasher<uint64_t>> m_fullKeys;
    std::unordered_map<uint64_t, Validator, NoHashHasher<uint64_t>> m_validators;
    std::unordered_map<uint64_t, std::pair<matjson::Value, matjson::Value>, NoHashHasher<uint64_t>> m_limits;

    SettingsManager();

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