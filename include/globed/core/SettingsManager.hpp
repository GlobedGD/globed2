#pragma once

#include "../config.hpp"
#include "../util/singleton.hpp"
#include "../util/assert.hpp"
#include "ValueManager.hpp"

#include <Geode/utils/terminate.hpp>
#include <Geode/loader/Log.hpp>
#include <Geode/utils/function.hpp>

namespace globed {

// Enums for some settings

enum class InvitesFrom : int {
    Everyone = 0,
    Friends = 1,
    Nobody = 2,
};

enum class PreferConnection : int {
    Auto = 0,
    Tcp = 1,
    Udp = 2,
    Quic = 3,
    WebSocket = 4,
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
    using Validator = geode::Function<bool(const matjson::Value&)>;
    using ListenCallback = geode::Function<void(const matjson::Value&)>;

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

    bool listenForChangesRaw(
        std::string_view key,
        geode::Function<void(const matjson::Value&)> callback
    );

    template <typename T>
    bool listenForChanges(
        std::string_view key,
        geode::Function<void(const T&)> callback
    ) {
        return this->listenForChangesRaw(key, [cb = std::move(callback)](const matjson::Value& value) mutable {
            if (auto res = value.as<T>()) {
                cb(*res);
            }
        });
    }

    /// This will throw if the setting is not found or of the wrong type
    template <typename T>
    T getAndListenForChanges(
        std::string_view key,
        geode::Function<void(const T&)> callback
    ) {
        if (this->listenForChanges<T>(key, std::move(callback))) {
            return this->setting<T>(key);
        }
        throw std::runtime_error(fmt::format("setting not found with key {}", key));
    }

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


// Setting keys

namespace Setting {

namespace Preload {
    inline constexpr auto Enabled = "core.preload.enabled";
    inline constexpr auto Defer = "core.preload.defer";
    inline constexpr auto ForcePreload = "core.preload.force-preload";
    inline constexpr auto BatchSize = "core.preload.batch-size";
    inline constexpr auto UsePbos = "core.preload.use-pbos";
    inline constexpr auto UseDirectDecode = "core.preload.use-direct-decode";
}

namespace General {
    inline constexpr auto Autoconnect = "core.autoconnect";
    inline constexpr auto StreamerMode = "core.streamer-mode";
    inline constexpr auto InvitesFrom = "core.invites-from";
}

namespace Editor {
    inline constexpr auto Enabled = "core.editor.enabled";
}

namespace Keybinds {
    inline constexpr auto VoiceChat = "core.keybinds.voice-chat";
    inline constexpr auto Deafen = "core.keybinds.deafen";
    inline constexpr auto HidePlayers = "core.keybinds.hide-players";
    inline constexpr auto Emote0 = "core.keybinds.emote-0";
    inline constexpr auto Emote1 = "core.keybinds.emote-1";
    inline constexpr auto Emote2 = "core.keybinds.emote-2";
    inline constexpr auto Emote3 = "core.keybinds.emote-3";
    inline constexpr auto Emote4 = "core.keybinds.emote-4";
    inline constexpr auto Emote5 = "core.keybinds.emote-5";
    inline constexpr auto Emote6 = "core.keybinds.emote-6";
    inline constexpr auto Emote7 = "core.keybinds.emote-7";
}

namespace Ui {
    inline constexpr auto AllowCustomServers = "core.ui.allow-custom-servers";
    inline constexpr auto IncreaseLevelList = "core.ui.increase-level-list";
    inline constexpr auto CompressedPlayerCount = "core.ui.compressed-player-count";
    inline constexpr auto ColorblindMode = "core.ui.colorblind-mode";
    inline constexpr auto DisableNotices = "core.ui.disable-notices";
}

namespace Player {
    inline constexpr auto Opacity = "core.player.opacity";
    inline constexpr auto QuickChatEnabled = "core.player.quick-chat-enabled";
    inline constexpr auto QuickChatSfx = "core.player.quick-chat-sfx";
    inline constexpr auto QuickChatSfxVolume = "core.player.quick-chat-sfx-volume";
    inline constexpr auto EmoteOpacity = "core.player.emote-opacity";
    inline constexpr auto ShowNames = "core.player.show-names";
    inline constexpr auto DualName = "core.player.dual-name";
    inline constexpr auto NameOpacity = "core.player.name-opacity";
    inline constexpr auto ForceVisibility = "core.player.force-visibility";
    inline constexpr auto HideNearbyClassic = "core.player.hide-nearby-classic";
    inline constexpr auto HideNearbyPlat = "core.player.hide-nearby-plat";
    inline constexpr auto HidePracticing = "core.player.hide-practicing";
    inline constexpr auto StatusIcons = "core.player.status-icons";
    inline constexpr auto RotateNames = "core.player.rotate-names";
    inline constexpr auto DeathEffects = "core.player.death-effects";
    inline constexpr auto DefaultDeathEffects = "core.player.default-death-effects";
    inline constexpr auto BlacklistedPlayers = "core.player.blacklisted-players";
    inline constexpr auto WhitelistedPlayers = "core.player.whitelisted-players";
    inline constexpr auto HiddenPlayers = "core.player.hidden-players";
}

namespace Level {
    inline constexpr auto ProgressIndicators = "core.level.progress-indicators";
    inline constexpr auto ProgressIndicatorsPlat = "core.level.progress-indicators-plat";
    inline constexpr auto ProgressOpacity = "core.level.progress-opacity";
    inline constexpr auto ForceProgressbar = "core.level.force-progressbar";
    inline constexpr auto SelfStatusIcons = "core.level.self-status-icons";
    inline constexpr auto SelfName = "core.level.self-name";
    inline constexpr auto FixProgressBar = "core.level.fix-progress-bar";
    inline constexpr auto VoiceOverlay = "core.level.voice-overlay";
    inline constexpr auto VoiceOverlayThreshold = "core.level.voice-overlay-threshold";
    inline constexpr auto VoiceOverlayPosition = "core.level.voice-overlay-position";
    inline constexpr auto VoiceOverlayPadY = "core.level.voice-overlay-pad-y";
}

namespace Overlay {
    inline constexpr auto Enabled = "core.overlay.enabled";
    inline constexpr auto Opacity = "core.overlay.opacity";
    inline constexpr auto AlwaysShow = "core.overlay.always-show";
    inline constexpr auto Position = "core.overlay.position";
}

namespace Audio {
    inline constexpr auto VoiceChatEnabled = "core.audio.voice-chat-enabled";
    inline constexpr auto InputDevice = "core.audio.input-device";
    inline constexpr auto InputDeviceGuid = "core.audio.input-device-guid";
    inline constexpr auto VoiceLoopback = "core.audio.voice-loopback";
    inline constexpr auto OverlayingOverlay = "core.audio.overlaying-overlay";
    inline constexpr auto BufferSize = "core.audio.buffer-size";
    inline constexpr auto PlaybackVolume = "core.audio.playback-volume";
    inline constexpr auto VoiceProximity = "core.audio.voice-proximity";
    inline constexpr auto ClassicProximity = "core.audio.classic-proximity";
    inline constexpr auto DeafenNotification = "core.audio.deafen-notification";
    inline constexpr auto OnlyFriends = "core.audio.only-friends";
}

namespace ModSettings {
    inline constexpr auto RememberPassword = "core.mod.remember-password";
}

namespace User {
    inline constexpr auto AllowUserSettings = "core.user.allow-user-settings";
    inline constexpr auto HideInLevels = "core.user.hide-in-levels";
    inline constexpr auto HideInMenus = "core.user.hide-in-menus";
    inline constexpr auto HideRoles = "core.user.hide-roles";
}

namespace Dev {
    inline constexpr auto PacketLossSim = "core.dev.packet-loss-sim";
    inline constexpr auto NetDebugLogs = "core.dev.net-debug-logs";
    inline constexpr auto NetStatDump = "core.dev.net-stat-dump";
    inline constexpr auto NetPreferProto = "core.dev.net-prefer-proto";
    inline constexpr auto NetUseIpv4 = "core.dev.net-use-ipv4";
    inline constexpr auto NetDontOverrideDns = "core.dev.net-dont-override-dns";
    inline constexpr auto FakeData = "core.dev.fake-data";
    inline constexpr auto CertVerification = "core.dev.cert-verification";
    inline constexpr auto GhostFollower = "core.dev.ghost-follower";
    inline constexpr auto ProfileFrameTime = "core.dev.profile-frame-time";
}
}

}
