#pragma once
#include <defs.hpp>

#define GSETTING(ty,name) ty name = _DefaultFor##name
// #define GDEFAULT(name, val) if (std::string_view(#name) == std::string_view(key)) return val
#define GDEFAULT(name, type, val) static constexpr type _DefaultFor##name = val;

// This class should only be accessed from the main thread.
class GlobedSettings : GLOBED_SINGLETON(GlobedSettings) {
public:
    struct Globed {
        GSETTING(uint32_t, tpsCap);
        GSETTING(int, audioDevice);
        GSETTING(bool, autoconnect); // todo unimpl
    };

    struct Overlay {
        GSETTING(float, overlayOpacity);
    };

    struct Communication {
        GSETTING(bool, voiceEnabled);
    };

    struct LevelUI {};
    struct Players {};
    struct Advanced {};

    struct Flags {
        bool seenSignupNotice = false;
    };

    Globed globed;
    Overlay overlay;
    Communication communication;
    LevelUI levelUi;
    Players players;
    Advanced advanced;
    Flags flags;

    GlobedSettings();

    void save();
    void reload();
    void resetToDefaults();
    void clear(const std::string_view key);

private:
    /* defaults */

    // globed
    GDEFAULT(tpsCap, uint32_t, 0);
    GDEFAULT(audioDevice, int, 0);
    GDEFAULT(autoconnect, bool, false);

    // overlay
    GDEFAULT(overlayOpacity, float, 0.5f);

    // communication
    GDEFAULT(voiceEnabled, bool, true);

    /* funcs */

    template <typename T>
    void store(const std::string_view key, const T& val) {
        geode::Mod::get()->setSavedValue(key, val);
    }

    bool has(const std::string_view key) {
        return geode::Mod::get()->getSaveContainer().contains(key);
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
    T loadOrDefault(const std::string_view key, const T defaultval) {
        return this->has(key) ? this->load<T>(key) : defaultval;
    }
};

#undef GSETTING
#undef GDEFAULT