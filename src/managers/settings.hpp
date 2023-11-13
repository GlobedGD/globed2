#pragma once
#include <defs.hpp>

struct CachedSettings {
    bool test;
};

// This class is only guaranteed to be safe to use from the main thread.
class GlobedSettings {
public:
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