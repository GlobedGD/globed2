#pragma once

#include "Geode/cocos/robtop/keyboard_dispatcher/CCKeyboardDelegate.h"
#include <defs/geode.hpp>
#include <defs/util.hpp>
#include <data/basic.hpp>
#include <data/types/user.hpp>
#include <managers/keybinds.hpp>
#include <util/singleton.hpp>

#include <asp/thread/Thread.hpp>
#include <asp/sync/Channel.hpp>

/*
 * v2 save system.
 *
 * Primary difference from v1 is the support of multiple save slots.
 * Save slots can be given custom names to distinguish them from each other.
*/

class GlobedSettings : public SingletonLeakBase<GlobedSettings> {
    friend class SingletonLeakBase;
    GlobedSettings();

private:
    template <globed::ConstexprString _name>
    struct Arg {
        static inline constexpr decltype(_name) Name = _name;

        bool _value;

        Arg() : _value(false) {}
        Arg(bool v) : _value(v) {}

        operator bool() const {
            return _value;
        }

        void operator=(bool v) {
            _value = v;
        }

        void set(bool v) {
            _value = v;
        }
    };

public:
    // Launch args
    struct LaunchArgs {
        Arg<"globed-crt-fix"> crtFix;
        Arg<"globed-verbose-curl"> verboseCurl;
        Arg<"globed-skip-preload"> skipPreload;
        Arg<"globed-debug-preload"> debugPreload;
        Arg<"globed-skip-resource-check"> skipResourceCheck;
        Arg<"globed-tracing"> tracing;
        Arg<"globed-no-ssl-verification"> noSslVerification;
        Arg<"globed-fake-server-data"> fakeData;
        Arg<"globed-reset-settings"> resetSettings;
    };

private:
    static LaunchArgs _launchArgs;

    struct WorkerTask {
        matjson::Value data;
        std::filesystem::path path;
    };

    asp::Thread<> workerThread;
    asp::Channel<WorkerTask> workerChannel;
    size_t selectedSlot = 0;
    matjson::Value settingsContainer;
    std::filesystem::path saveSlotPath;
    bool forceResetSettings = false;

public:
    // Forcefully saves all settings to a file (in a worker thread, to prevent slowdowns)
    void save();

    // Reloads settings from the save file, if it exists
    void reload();

    void reset();

    // Selects a save slot, reloads settings.
    void switchSlot(size_t index);

    // Creates a new save slot, returns its index
    Result<size_t> createSlot();

    size_t getSelectedSlot();

    // Get the launch arguments for this session
    const LaunchArgs& launchArgs();

    struct SaveSlot {
        size_t index;
        std::string name;
    };

    // Return all created save slots
    std::vector<SaveSlot> getSaveSlots();

    Result<size_t> getNextFreeSlot();

    void deleteSlot(size_t index);
    void renameSlot(size_t index, std::string_view name);

    std::filesystem::path pathForSlot(size_t idx);
    matjson::Value getSettingContainer();

    void reloadFromContainer(const matjson::Value& container);

private:
    void migrateFromV1();


    Result<matjson::Value> readSlotData(size_t idx);
    Result<matjson::Value> readSlotData(const std::filesystem::path& path);

    Result<SaveSlot> readSlotMetadata(size_t idx);
    Result<SaveSlot> readSlotMetadata(const std::filesystem::path& path, size_t idx);

    void loadLaunchArguments();
    void deleteAllSaveSlotFiles();
    void pushWorkerTask();
    void resetNoSave();

    enum class TaskType {
        Save, Load, Reset, HardReset
    };

    void reflect(TaskType tt);

public:
    // Util classes for defining settings

    // T -> T, but float -> globed::ConstexprFloat, as floats cant be used as template arguments
    template <typename T>
    using TypeFixup = std::conditional_t<std::is_same_v<T, float>, globed::ConstexprFloat, T>;

    /* Setting class */

    template <
        typename InnerTyRaw,
        TypeFixup<InnerTyRaw> DefaultV
    >
    class Setting {
    protected:
        using InnerTy = decltype(DefaultV);

    public:
        constexpr static bool IsFloat = std::is_same_v<InnerTy, globed::ConstexprFloat>;
        constexpr static bool IsLimited = false;

        using Type = std::conditional_t<IsFloat, float, InnerTy>;
        constexpr static Type Default = DefaultV;

        Setting() : value(Default) {}

        void set(const Type& v) {
            value = v;
        }

        Type get() const {
            return value;
        }

        Type& ref() {
            if constexpr (IsFloat) {
                return value.ref();
            } else {
                return value;
            }
        }

        const Type& ref() const {
            if constexpr (IsFloat) {
                return value.ref();
            } else {
                return value;
            }
        }

        operator Type() {
            return get();
        }

        operator float() const requires (IsFloat) {
            return get().asFloat();
        }

        Setting& operator=(const Type& other) {
            value = other;
            GlobedSettings::get().save();
            return *this;
        }

    protected:
        InnerTy value;
    };

    template <
        typename InnerTyRaw,
        TypeFixup<InnerTyRaw> DefaultV,
        TypeFixup<InnerTyRaw> MinimumV,
        TypeFixup<InnerTyRaw> MaximumV
    >
    class LimitedSetting : public Setting<InnerTyRaw, DefaultV> {
    public:
        using Type = Setting<InnerTyRaw, DefaultV>::Type;
        using InnerTy = Setting<InnerTyRaw, DefaultV>::InnerTy;
        using Setting<InnerTyRaw, DefaultV>::IsFloat;
        using Setting<InnerTyRaw, DefaultV>::Default;

        constexpr static bool IsLimited = true;

        constexpr static Type Minimum = MinimumV;
        constexpr static Type Maximum = MaximumV;

        LimitedSetting() {
            this->value = Default;
        }

        void set(const Type& v) {
            this->value = this->clamp(v);
        }

        operator Type() {
            return this->get();
        }

        operator float() const requires (IsFloat) {
            return this->get().asFloat();
        }

        LimitedSetting& operator=(const Type& other) {
            this->set(other);
            GlobedSettings::get().save();
            return *this;
        }

    private:
        Type clamp(const Type& value) {
            if (value > Maximum) return Maximum;
            if (value < Minimum) return Minimum;
            return value;
        }
    };

    using Float = globed::ConstexprFloat;
    using Flag = Setting<bool, false>;

    // Settings themselves, split into categories
    // when adding settings, please remember to also add them at the bottom of this file

    enum class InvitesFrom : int {
        Everyone = 0,
        Friends = 1,
        Nobody = 2,
    };

    struct Globed {
        Setting<bool, true> autoconnect;
        LimitedSetting<int, 0, 0, 240> tpsCap;
        Setting<bool, true> preloadAssets;
        Setting<bool, false> deferPreloadAssets;
        LimitedSetting<int, (int)InvitesFrom::Friends, 0, 2> invitesFrom;
        Setting<int, cocos2d::enumKeyCodes::KEY_H> hidePlayersKey;
        Setting<bool, true> editorSupport;
        Setting<bool, false> increaseLevelList;
        Setting<int, 60000> fragmentationLimit;
        Setting<bool, false> compressedPlayerCount;
        Setting<bool, true> useDiscordRPC;
        Setting<bool, true> changelogPopups;
        Setting<bool, false> editorChanges;

        // hidden settings! no settings ui for them

        Setting<bool, false> pinnedLevelCollapsed;

        /* privacy settings */
        Setting<bool, false> isInvisible;
        Setting<bool, false> noInvites;
        Setting<bool, false> hideInGame;
        Setting<bool, false> hideRoles;
    };

    struct Overlay {
        Setting<bool, true> enabled;
        LimitedSetting<float, 0.3f, 0.f, 1.f> opacity;
        Setting<bool, true> hideConditionally;
        LimitedSetting<int, 3, 0, 3> position; // 0-3 topleft, topright, bottomleft, bottomright
    };

    struct Communication {
        Setting<bool, true> voiceEnabled;
        Setting<int, cocos2d::enumKeyCodes::KEY_V> voiceChatKey;
        Setting<int, cocos2d::enumKeyCodes::KEY_B> voiceDeafenKey;
        Setting<bool, true> voiceProximity;
        Setting<bool, false> classicProximity;
        LimitedSetting<float, 1.0f, 0.f, 2.f> voiceVolume;
        Setting<bool, false> onlyFriends;
        Setting<bool, true> lowerAudioLatency;
        Setting<int, 0> audioDevice;
        Setting<bool, true> deafenNotification;
        Setting<bool, false> voiceLoopback; // TODO unimpl
    };

    struct LevelUI {
        Setting<bool, true> progressIndicators;
        Setting<bool, true> progressPointers; // unused
        LimitedSetting<float, 1.0f, 0.f, 1.f> progressOpacity;
        Setting<bool, true> voiceOverlay;
        Setting<bool, false> forceProgressBar;
    };

    struct Players {
        LimitedSetting<float, 1.0f, 0.f, 1.f> playerOpacity;
        Setting<bool, true> showNames;
        Setting<bool, true> dualName;
        LimitedSetting<float, 1.0f, 0.f, 1.f> nameOpacity;
        Setting<bool, true> statusIcons;
        Setting<bool, true> deathEffects;
        Setting<bool, false> defaultDeathEffect;
        Setting<bool, false> hideNearby;
        Setting<bool, false> forceVisibility;
        Setting<bool, false> ownName;
        Setting<bool, false> rotateNames;
        Setting<bool, false> hidePracticePlayers;
    };

    struct Advanced {};

    struct Admin {
        Setting<bool, false> rememberPassword;
    };

    struct Flags {
        Flag seenSignupNotice;       // Obsolete
        Flag seenSignupNoticev2;
        Flag seenVoiceChatPTTNotice;
        Flag seenTeleportNotice;
        Flag seenAprilFoolsNotice;   // Obsolete
        Flag seenStatusNotice;       // Obsolete
        Flag seenGlobalTriggerGuide;
        Flag seenRoomOptionsSafeModeNotice;
    };

    Globed globed;
    Overlay overlay;
    Communication communication;
    LevelUI levelUi;
    Players players;
    Advanced advanced;
    Admin admin;
    Flags flags;

    inline UserPrivacyFlags getPrivacyFlags() {
        return UserPrivacyFlags {
            .hideFromLists = globed.isInvisible,
            .noInvites = globed.noInvites,
            .hideInGame = globed.hideInGame,
            .hideRoles = globed.hideRoles,
        };
    }

private:
    bool has(std::string_view key);
    bool hasFlag(std::string_view key);
    void clear(std::string_view key);
    void clearFlag(std::string_view key);

    template <typename T>
    void store(std::string_view key, const T& val) {
        settingsContainer[key] = val;
    }

    inline void storeFlag(std::string_view key, bool val) {
        Mod::get()->setSavedValue(key, val);
    }

    template <typename T>
    T load(std::string_view key) {
        return settingsContainer[key].template as<T>().unwrapOr(T());
    }

    bool loadFlag(std::string_view key) {
        return Mod::get()->getSavedValue<bool>(key);
    }

    // If setting is present, loads into `into`. Otherwise does nothing.
    template <typename T>
    void loadOptionalInto(std::string_view key, T& into) {
        if (this->has(key)) {
            into = this->load<T>(key);
        }
    }

    template <typename T = bool>
    void loadOptionalFlagInto(std::string_view key, T& into) {
        if (this->hasFlag(key)) {
            into = this->loadFlag(key);
        }
    }

    template <typename T>
    std::optional<T> loadOptional(std::string_view key) {
        return this->has(key) ? this->load<T>(key) : std::nullopt;
    }

    template <typename T>
    T loadOrDefault(std::string_view key, const T& defaultval) {
        return this->has(key) ? this->load<T>(key) : defaultval;
    }
};

/* Enable reflection */

// Launch args

GLOBED_SERIALIZABLE_STRUCT(GlobedSettings::LaunchArgs, (
    crtFix, verboseCurl, skipPreload, debugPreload, skipResourceCheck, tracing, noSslVerification, fakeData, resetSettings
));

// Settings

GLOBED_SERIALIZABLE_STRUCT(GlobedSettings::Globed, (
    autoconnect, tpsCap, preloadAssets, deferPreloadAssets, invitesFrom, editorSupport, increaseLevelList, fragmentationLimit, compressedPlayerCount, useDiscordRPC, editorChanges, changelogPopups, pinnedLevelCollapsed,
    isInvisible, noInvites, hideInGame, hideRoles
));

GLOBED_SERIALIZABLE_STRUCT(GlobedSettings::Overlay, (
    enabled, opacity, hideConditionally, position
));

GLOBED_SERIALIZABLE_STRUCT(GlobedSettings::Communication, (
    voiceEnabled, voiceChatKey, voiceDeafenKey, voiceProximity, classicProximity, voiceVolume, onlyFriends, lowerAudioLatency, audioDevice, deafenNotification, voiceLoopback
));

GLOBED_SERIALIZABLE_STRUCT(GlobedSettings::LevelUI, (
    progressIndicators, progressPointers, progressOpacity, voiceOverlay, forceProgressBar
));

GLOBED_SERIALIZABLE_STRUCT(GlobedSettings::Players, (
    playerOpacity, showNames, dualName, nameOpacity, statusIcons, deathEffects, defaultDeathEffect, hideNearby, forceVisibility, ownName, hidePracticePlayers, rotateNames
));

GLOBED_SERIALIZABLE_STRUCT(GlobedSettings::Advanced, ());

GLOBED_SERIALIZABLE_STRUCT(GlobedSettings::Admin, (
    rememberPassword
));

GLOBED_SERIALIZABLE_STRUCT(GlobedSettings::Flags, (
    seenSignupNotice, seenSignupNoticev2, seenVoiceChatPTTNotice, seenTeleportNotice, seenAprilFoolsNotice, seenStatusNotice, seenGlobalTriggerGuide, seenRoomOptionsSafeModeNotice
));

GLOBED_SERIALIZABLE_STRUCT(GlobedSettings, (
    globed, overlay, communication, levelUi, players, advanced, admin, flags
));
