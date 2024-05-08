#pragma once

#include <data/basic.hpp>
#include <defs/geode.hpp>
#include <defs/util.hpp>

template <typename InnerTy, InnerTy DefaultV>
class GlobedSetting {
public:
    constexpr static bool IsFloat = std::is_same_v<InnerTy, globed::ConstexprFloat>;

    using Type = std::conditional_t<IsFloat, float, InnerTy>;
    constexpr static Type Default = DefaultV;

    GlobedSetting() : value(Default) {}

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

    GlobedSetting& operator=(const Type& other) {
        value = other;
        return *this;
    }

private:
    InnerTy value;
};

class GlobedSettings : public SingletonBase<GlobedSettings> {
    friend class SingletonBase;
    GlobedSettings();

    using Float = globed::ConstexprFloat;
    using Flag = GlobedSetting<bool, false>;

public:
    /* When adding settings, please remember to also add them at the bottom of this file. */

    struct Globed {
        GlobedSetting<bool, true> autoconnect;
        GlobedSetting<int, 0> tpsCap;
        GlobedSetting<bool, true> preloadAssets;
        GlobedSetting<bool, false> deferPreloadAssets;
        GlobedSetting<bool, false> increaseLevelList;
        GlobedSetting<int, 60000> fragmentationLimit;
        GlobedSetting<bool, false> compressedPlayerCount;
    };

    struct Overlay {
        GlobedSetting<bool, true> enabled;
        GlobedSetting<Float, Float(0.3f)> opacity;
        GlobedSetting<bool, true> hideConditionally;
        GlobedSetting<int, 3> position; // 0-3 topleft, topright, bottomleft, bottomright
    };

    struct Communication {
        GlobedSetting<bool, true> voiceEnabled;
        GlobedSetting<bool, true> voiceProximity;
        GlobedSetting<bool, false> classicProximity;
        GlobedSetting<Float, Float(1.0f)> voiceVolume;
        GlobedSetting<bool, false> onlyFriends;
        GlobedSetting<bool, true> lowerAudioLatency;
        GlobedSetting<int, 0> audioDevice;
        GlobedSetting<bool, true> deafenNotification;
        GlobedSetting<bool, false> voiceLoopback; // TODO unimpl
    };

    struct LevelUI {
        GlobedSetting<bool, true> progressIndicators;
        GlobedSetting<bool, true> progressPointers; // unused
        GlobedSetting<Float, Float(1.0f)> progressOpacity;
        GlobedSetting<bool, true> voiceOverlay;
    };

    struct Players {
        GlobedSetting<Float, Float(1.0f)> playerOpacity;
        GlobedSetting<bool, true> showNames;
        GlobedSetting<bool, true> dualName;
        GlobedSetting<Float, Float(1.0f)> nameOpacity;
        GlobedSetting<bool, true> statusIcons;
        GlobedSetting<bool, true> deathEffects;
        GlobedSetting<bool, false> defaultDeathEffect;
        GlobedSetting<bool, false> hideNearby;
        GlobedSetting<bool, false> forceVisibility;
        GlobedSetting<bool, false> ownName;
        GlobedSetting<bool, false> hidePracticePlayers;
    };

    struct Advanced {};

    struct Admin {
        GlobedSetting<bool, false> rememberPassword;
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

    // Save all settings to the geode save container
    void save();

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
