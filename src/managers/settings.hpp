#pragma once

#include <data/basic.hpp>
#include <defs/geode.hpp>
#include <defs/util.hpp>

class GlobedSettings : public SingletonBase<GlobedSettings> {
    friend class SingletonBase;
    GlobedSettings();

public:
    // Save all settings to the geode save container
    void save();

private:
    /* Setting class */

    template <
        typename InnerTyRaw,
        std::conditional_t<std::is_same_v<InnerTyRaw, float>, globed::ConstexprFloat, InnerTyRaw> DefaultV
    >
    class Setting {
        using InnerTy = decltype(DefaultV);

    public:
        constexpr static bool IsFloat = std::is_same_v<InnerTy, globed::ConstexprFloat>;

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

        operator float() const requires (std::is_same_v<Type, globed::ConstexprFloat>) {
            return get().asFloat();
        }

        Setting& operator=(const Type& other) {
            value = other;
            GlobedSettings::get().save();
            return *this;
        }

    private:
        InnerTy value;
    };

    using Float = globed::ConstexprFloat;
    using Flag = Setting<bool, false>;

public:
    // Settings themselves, split into categories
    // when adding settings, please remember to also add them at the bottom of this file

    struct Globed {
        Setting<bool, true> autoconnect;
        Setting<int, 0> tpsCap;
        Setting<bool, true> preloadAssets;
        Setting<bool, false> deferPreloadAssets;
        Setting<bool, false> increaseLevelList;
        Setting<int, 60000> fragmentationLimit;
        Setting<bool, false> compressedPlayerCount;
    };

    struct Overlay {
        Setting<bool, true> enabled;
        Setting<float, 0.3f> opacity;
        Setting<bool, true> hideConditionally;
        Setting<int, 3> position; // 0-3 topleft, topright, bottomleft, bottomright
    };

    struct Communication {
        Setting<bool, true> voiceEnabled;
        Setting<bool, true> voiceProximity;
        Setting<bool, false> classicProximity;
        Setting<float, 1.0f> voiceVolume;
        Setting<bool, false> onlyFriends;
        Setting<bool, true> lowerAudioLatency;
        Setting<int, 0> audioDevice;
        Setting<bool, true> deafenNotification;
        Setting<bool, false> voiceLoopback; // TODO unimpl
    };

    struct LevelUI {
        Setting<bool, true> progressIndicators;
        Setting<bool, true> progressPointers; // unused
        Setting<float, 1.0f> progressOpacity;
        Setting<bool, true> voiceOverlay;
    };

    struct Players {
        Setting<float, 1.0f> playerOpacity;
        Setting<bool, true> showNames;
        Setting<bool, true> dualName;
        Setting<float, 1.0f> nameOpacity;
        Setting<bool, true> statusIcons;
        Setting<bool, true> deathEffects;
        Setting<bool, false> defaultDeathEffect;
        Setting<bool, false> hideNearby;
        Setting<bool, false> forceVisibility;
        Setting<bool, false> ownName;
        Setting<bool, false> hidePracticePlayers;
    };

    struct Advanced {};

    struct Admin {
        Setting<bool, false> rememberPassword;
    };

    struct Flags {
        Flag seenSignupNotice;
        Flag seenSignupNoticev2;
        Flag seenVoiceChatPTTNotice;
        Flag seenTeleportNotice;
        Flag seenAprilFoolsNotice;
    };

    Globed globed;
    Overlay overlay;
    Communication communication;
    LevelUI levelUi;
    Players players;
    Advanced advanced;
    Admin admin;
    Flags flags;

    // Reset everything, including flags
    void hardReset();

    // Reset all settings, excluding flags
    void reset();

    // Reload all settings from the geode save container
    void reload();

    enum class TaskType {
        SaveSettings, LoadSettings, ResetSettings, HardResetSettings
    };

    void reflect(TaskType type);

    bool has(const std::string_view key);
    void clear(const std::string_view key);

    template <typename T>
    void store(const std::string_view key, const T& val) {
        geode::Mod::get()->setSavedValue(key, val);
    }

    template <typename T>
    T load(const std::string_view key) {
        return geode::Mod::get()->getSavedValue<T>(key);
    }

    // If setting is present, loads into `into`. Otherwise does nothing.
    template <typename T>
    void loadOptionalInto(const std::string_view key, T& into) {
        if (this->has(key)) {
            into = this->load<T>(key);
        }
    }

    template <typename T>
    std::optional<T> loadOptional(const std::string_view key) {
        return this->has(key) ? this->load<T>(key) : std::nullopt;
    }

    template <typename T>
    T loadOrDefault(const std::string_view key, const T& defaultval) {
        return this->has(key) ? this->load<T>(key) : defaultval;
    }
};

/* Enable reflection */

GLOBED_SERIALIZABLE_STRUCT(GlobedSettings::Globed, (
    autoconnect, tpsCap, preloadAssets, deferPreloadAssets, increaseLevelList, fragmentationLimit, compressedPlayerCount
));

GLOBED_SERIALIZABLE_STRUCT(GlobedSettings::Overlay, (
    enabled, opacity, hideConditionally, position
));

GLOBED_SERIALIZABLE_STRUCT(GlobedSettings::Communication, (
    voiceEnabled, voiceProximity, classicProximity, voiceVolume, onlyFriends, lowerAudioLatency, deafenNotification, voiceLoopback
));

GLOBED_SERIALIZABLE_STRUCT(GlobedSettings::LevelUI, (
    progressIndicators, progressPointers, progressOpacity, voiceOverlay
));

GLOBED_SERIALIZABLE_STRUCT(GlobedSettings::Players, (
    playerOpacity, showNames, dualName, nameOpacity, statusIcons, deathEffects, defaultDeathEffect, hideNearby, forceVisibility, ownName, hidePracticePlayers
));

GLOBED_SERIALIZABLE_STRUCT(GlobedSettings::Advanced, ());

GLOBED_SERIALIZABLE_STRUCT(GlobedSettings::Admin, (
    rememberPassword
));

GLOBED_SERIALIZABLE_STRUCT(GlobedSettings::Flags, (
    seenSignupNotice, seenSignupNoticev2, seenVoiceChatPTTNotice, seenTeleportNotice, seenAprilFoolsNotice
));

GLOBED_SERIALIZABLE_STRUCT(GlobedSettings, (
    globed, overlay, communication, levelUi, players, advanced, admin, flags
));
