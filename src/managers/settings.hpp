#pragma once
#include <defs.hpp>

#define GSETTING(ty,name) ty name = defaultFor<ty>(#name)
#define GDEFAULT(name, val) if (std::string_view(#name) == std::string_view(key)) return val

// This class should only be accessed from the main thread.
class GlobedSettings : GLOBED_SINGLETON(GlobedSettings) {
public:
    struct Globed {
        GSETTING(uint32_t, tpsCap);
        GSETTING(int, audioDevice);
        GSETTING(bool, autoconnect); // todo unimpl
    };

    struct Overlay {};
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
    void clear(const std::string& key);

private:
    template <typename T>
    static constexpr T defaultFor(const char* key) {
        GDEFAULT(tpsCap, 0);
        GDEFAULT(audioDevice, 0);
        GDEFAULT(autoconnect, false);

        GDEFAULT(voiceEnabled, true);

        geode::log::warn("warning: invalid default opt was requested: {}", key);

        return {};
    }

    template <typename T>
    void store(const std::string& key, const T& val) {
        geode::Mod::get()->setSavedValue(key, val);
    }

    bool has(const std::string& key) {
        return geode::Mod::get()->hasSavedValue(key)
            && !geode::Mod::get()->getSaveContainer().as_object()[key].is_null();
    }

    template <typename T>
    T load(const std::string& key) {
        return geode::Mod::get()->getSavedValue<T>(key);
    }

    // If setting is present, loads into `into`. Otherwise does nothing.
    template <typename T>
    void loadOptionalInto(const std::string& key, T& into) {
        if (this->has(key)) {
            auto val = this->load<T>(key);
        }
    }

    template <typename T>
    std::optional<T> loadOptional(const std::string& key) {
        return this->has(key) ? this->load<T>(key) : std::nullopt;
    }

    template <typename T>
    T loadOrDefault(const std::string& key, const T defaultval) {
        return this->has(key) ? this->load<T>(key) : defaultval;
    }
};

#undef GSETTING
#undef GDEFAULT