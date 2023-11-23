#pragma once
#include <defs.hpp>
#include <util/sync.hpp>

struct CachedSettings {
    bool test;
};

// Besides `getCached()`, this class is not thread safe (reason: getSavedValue/setSavedValue)
class GlobedSettings {
    GLOBED_SINGLETON(GlobedSettings);
    GlobedSettings();

    // directly set and save the setting as json
    template <typename T>
    inline void set(const std::string& key, const T& elem, bool refresh = true) {
        geode::Mod::get()->setSavedValue<T>("gsetting-" + key, elem);
        
        if (refresh) {
            this->refreshCache();
        }
    }

    // directly get the setting as json
    template <typename T>
    inline T get(const std::string& key) {
        return geode::Mod::get()->getSavedValue<T>("gsetting-" + key);
    }

    // get cached settings for performance
    CachedSettings getCached();

    void refreshCache();

    bool getFlag(const std::string& key);
    void setFlag(const std::string& key, bool state = true);

private:
    util::sync::WrappingMutex<CachedSettings> _cache;
};