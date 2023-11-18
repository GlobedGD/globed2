#pragma once
#include <defs.hpp>
#include <util/sync.hpp>

struct CachedSettings {
    bool test;
};

// Besides `getCached()`, this class is not guaranteed to be fully thread safe.
class GlobedSettings {
    GLOBED_SINGLETON(GlobedSettings);
    GlobedSettings();

    // directly set and save the setting as json
    template <typename T>
    void set(const std::string& key, const T& elem, bool refresh = true) {
        geode::Mod::get()->setSavedValue<T>("gsetting-" + key);
        
        if (refresh) {
            refreshCache();
        }
    }

    // directly get the setting as json
    template <typename T>
    T get(const std::string& key) {
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