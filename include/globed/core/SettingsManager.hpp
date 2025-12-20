#pragma once

#include <globed/config.hpp>
#include <globed/util/singleton.hpp>
#include <globed/util/assert.hpp>
#include "ValueManager.hpp"

#include <Geode/utils/terminate.hpp>
#include <Geode/loader/Log.hpp>
#include <std23/move_only_function.h>

namespace globed {

// Enums for some settings

enum class InvitesFrom : int {
    Everyone = 0,
    Friends = 1,
    Nobody = 2,
};

// Settings implementation

template <typename T>
class SettingAccessor {
public:
    SettingAccessor(std::string_view key);

    operator T() const;
    T value() const;

    SettingAccessor& operator=(T value);

private:
    uint64_t hash;
    std::string_view key;
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

struct SaveSlotMeta {
    size_t id;
    std::string name;
    bool active;
};

class GLOBED_DLL SettingsManager : public SingletonBase<SettingsManager> {
public:
    using Validator = std23::move_only_function<bool(const matjson::Value&)>;
    using ListenCallback = std23::move_only_function<void(const matjson::Value&)>;

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
            geode::utils::terminate(fmt::format("setting with hash {} has no default value or the default value is of an invalid type", hash));
        }
    }

    template <typename T>
    void setSettingRaw(uint64_t hash, T&& value) {
        if (!this->hasSetting(hash)) {
            // internal error, means we used a wrong id somewhere
            geode::utils::terminate(fmt::format("setting not found with hash {}", hash));
        }

        matjson::Value val = std::forward<T>(value);
        auto it = m_validators.find(hash);
        if (it != m_validators.end()) {
            if (!it->second(val)) {
                geode::log::warn("Failed to save setting {}, validation failed: {}", m_fullKeys[hash], val.dump(matjson::NO_INDENTATION));
                return;
            }
        }

        m_settings[hash] = val;
        auto callbacksIt = m_callbacks.find(hash);
        if (callbacksIt != m_callbacks.end()) {
            for (auto& cb : callbacksIt->second) {
                cb(val);
            }
        }
        // ValueManager::get().set(m_fullKeys[hash], std::move(val));

        if (m_activeSaveSlot >= m_saveSlots.size()) {
            GLOBED_ASSERT(m_activeSaveSlot == 0);
            m_saveSlots.emplace_back(matjson::Value::object());
        }

        auto& slot = m_saveSlots[m_activeSaveSlot];
        slot.set(m_fullKeys[hash], std::move(val));
    }

    std::optional<std::pair<matjson::Value, matjson::Value>> getLimits(std::string_view key);

    bool hasSetting(uint64_t hash);

    /// Registers a new setting. This cannot be called after the SettingsManager is frozen,
    /// which happens at the end of the loading phase when the core is initialized.
    void registerSetting(
        std::string_view key,
        matjson::Value defaultVal
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

    bool listenForChanges(
        std::string_view key,
        std23::move_only_function<void(const matjson::Value&)> callback
    );

    void commitSlotsToDisk();

    void reloadSetting(std::string_view fullKey);

    /// Resets the data in the current save slot
    void reset();

    std::vector<SaveSlotMeta> getSaveSlots();
    void renameSaveSlot(size_t id, std::string_view newName);
    void deleteSaveSlot(size_t id);
    void createSaveSlot();
    void switchToSaveSlot(size_t id);
    inline size_t getActiveSaveSlot() const { return m_activeSaveSlot; }

    bool isPlayerBlacklisted(int id);
    bool isPlayerWhitelisted(int id);
    bool isPlayerHidden(int id);
    void blacklistPlayer(int id);
    void whitelistPlayer(int id);
    void setPlayerHidden(int id, bool hidden);
    void refreshPlayerLists();
    void commitPlayerLists();

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
    std::unordered_map<uint64_t, std::vector<ListenCallback>, NoHashHasher<uint64_t>> m_callbacks;
    std::unordered_map<uint64_t, std::pair<matjson::Value, matjson::Value>, NoHashHasher<uint64_t>> m_limits;

    std::filesystem::path m_slotDir;
    std::vector<matjson::Value> m_saveSlots;
    size_t m_activeSaveSlot = 0;

    std::unordered_set<int> m_whitelisted, m_blacklisted, m_hidden;

    SettingsManager();

    void freeze();

    void loadSaveSlots();
    void reloadFromSlot();
    void migrateOldSettings();

    std::optional<matjson::Value> findSettingInSaveSlot(std::string_view key);

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
SettingAccessor<T>::SettingAccessor(std::string_view key) : hash(SettingsManager::keyHash(key)), key(key) {}

template <typename T>
SettingAccessor<T>::operator T() const {
    return this->value();
}

template <typename T>
T SettingAccessor<T>::value() const {
#ifdef GLOBED_DEBUG
    try {
        return SettingsManager::get().getSettingRaw<T>(hash);
    } catch (const std::exception& e) {
        geode::log::error("Invalid setting '{}': {}", key, e.what());
        throw;
    }
#else
    return SettingsManager::get().getSettingRaw<T>(hash);
#endif
}

template <typename T>
SettingAccessor<T>& SettingAccessor<T>::operator=(T value) {
#ifdef GLOBED_DEBUG
    try {
        SettingsManager::get().setSettingRaw<T>(hash, std::move(value));
    } catch (const std::exception& e) {
        geode::log::error("Invalid setting '{}': {}", key, e.what());
        throw;
    }
#else
    SettingsManager::get().setSettingRaw<T>(hash, std::move(value));
#endif
    return *this;
}

}