#pragma once
#include <defs.hpp>

struct CachedSettings {
    bool test;
};

// Getting a setting is thread-safe. `set`, `refreshCache` and `getCached` are only safe to call from the main thread.
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

private:
    CachedSettings cache;
};