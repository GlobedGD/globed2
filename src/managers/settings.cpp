#include "settings.hpp"

GLOBED_SINGLETON_DEF(GlobedSettings)
GlobedSettings::GlobedSettings() {
    this->refreshCache();
}

void GlobedSettings::reset(const std::string& key) {
    this->setFlag("_gset_-" + key, false);
    this->refreshCache();
}

GlobedSettings::CachedSettings GlobedSettings::getCached() {
    return *_cache.lock();
}

bool GlobedSettings::getFlag(const std::string& key) {
    return geode::Mod::get()->getSavedValue<int64_t>("gflag-" + key) == 2;
}

void GlobedSettings::setFlag(const std::string& key, bool state) {
    geode::Mod::get()->setSavedValue("gflag-" + key, static_cast<int64_t>(state ? 2 : 0));
}