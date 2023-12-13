#pragma once
#include <defs.hpp>
#include <util/sync.hpp>

#define MAKE_DEFAULT(_key, value) if (key == (_key)) return (value);

// Besides `getCached()`, this class is not thread safe (reason: Mod::getSavedValue/Mod::setSavedValue)
class GlobedSettings : GLOBED_SINGLETON(GlobedSettings) {
public:
    GlobedSettings();

    struct CachedSettings;

    /*
    * ok i know this is a mess but here's a rundown of how the settings work:
    * say we have a setting that is an int called 'hello'.
    * when retreiving, we first attempt to read `gflag-_gset_-hello` via getFlag()
    * if the flag is false, we return a default value (defaults reside in `getValue`).
    * otherwise, we return `gsetting-hello`.
    *
    * upon saving, we explicitly set the flag described earlier to true and save the value.
    * i hope this isn't confusing!
    */

    // directly set and save the setting as json
    template <typename T>
    inline void setValue(const std::string& key, const T& elem, bool refresh = true) {
        this->setFlag("_gset_-" + key);
        geode::Mod::get()->setSavedValue<T>("gsetting-" + key, elem);

        if (refresh) {
            _cache.lock()->refresh(*this);
        }
    }

    // directly get the setting as json, or return a default value if not set
    template <typename T>
    inline T getValue(const std::string& key, T defaultopt = T{}) {
        if (this->getFlag("_gset_-" + key)) {
            return geode::Mod::get()->getSavedValue<T>("gsetting-" + key);
        } else {
            return defaultopt;
        }
    }

    // reset a setting to its default value
    void reset(const std::string& key, bool refreshCache = true);

    // cached settings for performance & thread safety
    CachedSettings getCached();

    bool getFlag(const std::string& key);
    void setFlag(const std::string& key, bool state = true);

    class CachedSettings {
    public:
        void refresh(GlobedSettings&);

        struct Globed {
            uint32_t tpsCap;
            int audioDevice;
            bool autoconnect; // TODO unimpl
        };

        struct Overlay {};
        struct Communication {
            bool voiceEnabled;
        };

        struct LevelUI {};
        struct Players {};
        struct Advanced {};

        Globed globed;
        Overlay overlay;
        Communication communication;
        LevelUI levelUi;
        Players players;
        Advanced advanced;
    };

    // reset all settings to their default values
    inline void resetAll() {
        static const char* settings[] = {
            "tps-cap",
            "audio-device",
            "autoconnect",

            "comms-voice-enabled",
        };

        for (auto* setting : settings) {
            this->reset(setting, false);
        }

        _cache.lock()->refresh(*this);
    }

private:
    util::sync::WrappingMutex<CachedSettings> _cache;
};

#undef MAKE_DEFAULT