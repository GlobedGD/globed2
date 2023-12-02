#pragma once
#include <defs.hpp>
#include <util/sync.hpp>

struct CachedSettings {
    uint32_t serverTps;
};

#define MAKE_DEFAULT(_key, value) if (key == (_key)) return (value);

// Besides `getCached()`, this class is not thread safe (reason: Mod::getSavedValue/Mod::setSavedValue)
class GlobedSettings {
    GLOBED_SINGLETON(GlobedSettings);
    GlobedSettings();

    /*
    * ok i know this is a mess but here's a rundown of how the settings work:
    * say we have a setting that is an int called 'hello'.
    * when retreiving, we first attempt to read `gflag-_gset_-hello` via getFlag()
    * if the flag is false, we return a default value (defaults reside in `get`).
    * otherwise, we return `gsetting-hello`.
    *
    * upon saving, we explicitly set the flag described earlier to true and save the value.
    * i hope this isn't confusing!
    */

    // directly set and save the setting as json
    template <typename T>
    inline void set(const std::string& key, const T& elem, bool refresh = true) {
        this->setFlag("_gset_-" + key);
        geode::Mod::get()->setSavedValue<T>("gsetting-" + key, elem);

        if (refresh) {
            this->refreshCache();
        }
    }

    // directly get the setting as json
    template <typename T>
    inline T get(const std::string& key) {
        if (this->getFlag("_gset_-" + key)) {
            return geode::Mod::get()->getSavedValue<T>("gsetting-" + key);
        } else {
            MAKE_DEFAULT("server-tps", (uint32_t)30)

            return T{};
        }
    }

    // reset a setting to its default value
    void reset(const std::string& key);

    // get cached settings for performance
    CachedSettings getCached();

    void refreshCache();

    bool getFlag(const std::string& key);
    void setFlag(const std::string& key, bool state = true);

private:
    util::sync::WrappingMutex<CachedSettings> _cache;
};

#undef MAKE_DEFAULT