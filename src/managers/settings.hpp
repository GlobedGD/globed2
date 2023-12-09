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
            this->refreshCache();
        }
    }

    // reset a setting to its default value
    void reset(const std::string& key);

    // cached settings for performance & thread safety
    CachedSettings getCached();

    bool getFlag(const std::string& key);
    void setFlag(const std::string& key, bool state = true);

    /* for ease of modification, all settings and their defaults are defined in the following struct and 3 functions. */

    struct CachedSettings {
        uint32_t tpsCap;
    };

    // directly get the setting as json
    template <typename T>
    inline T getValue(const std::string& key) {
        if (this->getFlag("_gset_-" + key)) {
            return geode::Mod::get()->getSavedValue<T>("gsetting-" + key);
        } else {
            MAKE_DEFAULT("tps-cap", (uint32_t)0)

            return T{};
        }
    }

    // reset all settings to their default values
    inline void resetAll() {
        static const char* settings[] = {
            "tps-cap",
        };
        for (auto* setting : settings) {
            this->reset(setting);
        }

        this->refreshCache();
    }

    inline void refreshCache() {
        auto cache = _cache.lock();
        cache->tpsCap = this->getValue<uint32_t>("tps-cap");
    }

private:
    util::sync::WrappingMutex<CachedSettings> _cache;
};

#undef MAKE_DEFAULT