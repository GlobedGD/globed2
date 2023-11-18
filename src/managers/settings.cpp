#include "settings.hpp"

GLOBED_SINGLETON_DEF(GlobedSettings)
GlobedSettings::GlobedSettings() {
    refreshCache();
}

CachedSettings GlobedSettings::getCached() {
    return cache;
}

void GlobedSettings::refreshCache() {
    cache.test = get<bool>("test");
}

bool GlobedSettings::getFlag(const std::string& key) {
    return geode::Mod::get()->getSavedValue<int64_t>("gflag-" + key) == 2;
}

void GlobedSettings::setFlag(const std::string& key, bool state) {
    geode::Mod::get()->setSavedValue("gflag-" + key, static_cast<int64_t>(state ? 2 : 0));
}