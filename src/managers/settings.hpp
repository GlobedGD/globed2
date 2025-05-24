#pragma once

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
        Arg<"globed-net-dump"> netDump;
        Arg<"globed-verbose-curl"> verboseCurl;
        Arg<"globed-skip-preload"> skipPreload;
        Arg<"globed-debug-preload"> debugPreload;
        Arg<"globed-skip-resource-check"> skipResourceCheck;
        Arg<"globed-tracing"> tracing;
        Arg<"globed-no-ssl-verification"> noSslVerification;
        Arg<"globed-fake-server-data"> fakeData;
        Arg<"globed-reset-settings"> resetSettings;
        Arg<"globed-dev-stuff"> devStuff;
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

    /* Base of all setting classes */

    template <
        typename StoredT,      // internally stored type, cannot be float or enum or anything like that
        typename EffectiveT,   // effective type, i.e. the type user will deal with (can be float, enum)
        typename SerializedT,
        StoredT DefaultV,
        bool DoLimit,
        StoredT MinV,
        StoredT MaxV
    >
    class BaseSetting {
    public:
        constexpr static bool IsFloat = std::is_same_v<StoredT, globed::ConstexprFloat>;
        constexpr static bool IsLimited = DoLimit;

        using Type = EffectiveT;
        using StoredType = StoredT;
        using SerializedType = SerializedT;

        constexpr static EffectiveT Default = static_cast<EffectiveT>(DefaultV);
        constexpr static StoredT Minimum = MinV;
        constexpr static StoredT Maximum = MaxV;

        BaseSetting() : value(DefaultV) {}

        void set(const Type& v) {
            if constexpr (IsLimited) {
                value = this->clamp(static_cast<StoredT>(v));
            } else {
                value = static_cast<StoredT>(v);
            }
        }

        Type get() const {
            return static_cast<Type>(value);
        }

        Type& ref() {
            if constexpr (IsFloat) {
                return value.ref();
            } else if constexpr (std::is_same_v<StoredT, Type>) {
                return value;
            } else {
                static_assert(sizeof(StoredT) == sizeof(Type), "stored and effective type of BaseSetting must be of the same size if they are different!");
                return *reinterpret_cast<Type*>(&value);
            }
        }

        Type* ptr() {
            return &this->ref();
        }

        const Type& ref() const {
            return const_cast<BaseSetting*>(this)->ref();
        }

        const Type* ptr() const {
            return &this->ref();
        }

        operator Type() const {
            return this->get();
        }

        BaseSetting& operator=(const Type& other) {
            this->set(other);
            GlobedSettings::get().save();
            return *this;
        }

    protected:
        StoredT value;

        StoredT clamp(const StoredT& value) {
            if (value > Maximum) return Maximum;
            if (value < Minimum) return Minimum;
            return value;
        }
    };

    template <typename TRaw, TypeFixup<TRaw> DefaultV>
    using Setting = BaseSetting<TypeFixup<TRaw>, TRaw, TRaw, DefaultV, false, TypeFixup<TRaw>{}, TypeFixup<TRaw>{}>;

    template <typename TRaw, TypeFixup<TRaw> DefaultV, TypeFixup<TRaw> MinV, TypeFixup<TRaw> MaxV>
    using LimitedSetting = BaseSetting<TypeFixup<TRaw>, TRaw, TRaw, DefaultV, true, MinV, MaxV>;

    template <typename E, E DefaultV, typename U = std::underlying_type_t<E>>
    using EnumSetting = BaseSetting<U, E, U, static_cast<U>(DefaultV), false, U{}, U{}>;

    template <typename E, E DefaultV, E MinV, E MaxV, typename U = std::underlying_type_t<E>>
    using LimitedEnumSetting = BaseSetting<
        U, E, U,
        static_cast<U>(DefaultV),
        true,
        static_cast<U>(MinV),
        static_cast<U>(MaxV)
    >;

    template <cocos2d::enumKeyCodes DefaultV>
    using KeybindSetting = EnumSetting<cocos2d::enumKeyCodes, DefaultV>;

    // Settings themselves, split into categories
    // !! when adding settings, please remember to also add them at the bottom of this file !!

    using Float = globed::ConstexprFloat;
    using Flag = Setting<bool, false>;

    enum class InvitesFrom : int {
        Everyone = 0,
        Friends = 1,
        Nobody = 2,
    };

    struct Globed {
        Setting<bool, true> autoconnect;
        Setting<bool, true> preloadAssets;
        Setting<bool, false> deferPreloadAssets;
        LimitedEnumSetting<InvitesFrom, InvitesFrom::Friends, InvitesFrom::Everyone, InvitesFrom::Nobody> invitesFrom;
        Setting<bool, true> editorSupport;
        Setting<bool, false> increaseLevelList;
        Setting<int, 0> fragmentationLimit;
        Setting<bool, false> forceTcp;
        Setting<bool, false> showRelays;
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
        Setting<bool, true> voiceProximity;
        Setting<bool, false> classicProximity;
        LimitedSetting<float, 1.0f, 0.f, 2.f> voiceVolume;
        Setting<bool, false> onlyFriends;
        Setting<bool, true> lowerAudioLatency;
        Setting<bool, false> tcpAudio;
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

    struct Keys {
        KeybindSetting<cocos2d::enumKeyCodes::KEY_V> voiceChatKey;
        KeybindSetting<cocos2d::enumKeyCodes::KEY_B> voiceDeafenKey;
        KeybindSetting<cocos2d::enumKeyCodes::KEY_None> hidePlayersKey;
    };

    struct Flags {
        Flag seenSignupNotice;       // Obsolete
        Flag seenSignupNoticev2;
        Flag seenSignupNoticev3;
        Flag seenVoiceChatPTTNotice;
        Flag seenTeleportNotice;
        Flag seenAprilFoolsNotice;   // Obsolete
        Flag seenStatusNotice;       // Obsolete
        Flag seenGlobalTriggerGuide;
        Flag seenRoomOptionsSafeModeNotice;
        Flag seenSwagConnectionPopup;
        Flag seenRelayNotice;
    };

    Globed globed;
    Overlay overlay;
    Communication communication;
    LevelUI levelUi;
    Players players;
    Advanced advanced;
    Admin admin;
    Keys keys;
    Flags flags;
    Setting<bool, false> dummySetting;

    inline UserPrivacyFlags getPrivacyFlags() {
        return UserPrivacyFlags {
            .hideFromLists = globed.isInvisible,
            .noInvites = globed.noInvites,
            .hideInGame = globed.hideInGame,
            .hideRoles = globed.hideRoles,
            .tcpAudio = communication.tcpAudio,
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
    crtFix, netDump, verboseCurl, skipPreload, debugPreload, skipResourceCheck, tracing, noSslVerification, fakeData, resetSettings, devStuff
));

// Settings

GLOBED_SERIALIZABLE_STRUCT(GlobedSettings::Globed, (
    autoconnect, preloadAssets, deferPreloadAssets, invitesFrom, editorSupport, increaseLevelList, fragmentationLimit, forceTcp, showRelays,
    compressedPlayerCount, useDiscordRPC, editorChanges, changelogPopups, pinnedLevelCollapsed,
    isInvisible, noInvites, hideInGame, hideRoles
));

GLOBED_SERIALIZABLE_STRUCT(GlobedSettings::Overlay, (
    enabled, opacity, hideConditionally, position
));

GLOBED_SERIALIZABLE_STRUCT(GlobedSettings::Communication, (
    voiceEnabled, voiceProximity, classicProximity, voiceVolume, onlyFriends, lowerAudioLatency, tcpAudio, audioDevice, deafenNotification, voiceLoopback
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

GLOBED_SERIALIZABLE_STRUCT(GlobedSettings::Keys, (
    voiceChatKey, voiceDeafenKey, hidePlayersKey
));

GLOBED_SERIALIZABLE_STRUCT(GlobedSettings::Flags, (
    seenSignupNotice, seenSignupNoticev2, seenSignupNoticev3, seenVoiceChatPTTNotice, seenTeleportNotice,
    seenAprilFoolsNotice, seenStatusNotice, seenGlobalTriggerGuide, seenRoomOptionsSafeModeNotice,
    seenSwagConnectionPopup, seenRelayNotice
));

GLOBED_SERIALIZABLE_STRUCT(GlobedSettings, (
    globed, overlay, communication, levelUi, players, advanced, admin, keys, flags
));

// Net dump thingy

namespace globed {
    void _doNetDump(std::string_view str);

    template <typename... Args>
    GEODE_INLINE void netLog(fmt::format_string<Args...> fmt, Args&&... args) {
        static bool enabled = GlobedSettings::get().launchArgs().netDump;

        if (enabled) {
            _doNetDump(fmt::format(fmt, std::forward<Args>(args)...));
        }
    }
}
