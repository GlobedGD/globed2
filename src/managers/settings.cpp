#include "settings.hpp"

GlobedSettings::GlobedSettings() {
    _cache.lock()->refresh(*this);
}

void GlobedSettings::reset(const std::string& key, bool refreshCache) {
    this->setFlag("_gset_-" + key, false);
    if (refreshCache) {
        _cache.lock()->refresh(*this);
    }
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

void GlobedSettings::CachedSettings::refresh(GlobedSettings& gs) {
    this->globed.tpsCap = gs.getValue<uint32_t>("tps-cap", 0);
    this->globed.audioDevice = gs.getValue("audio-device", 0);
    this->globed.autoconnect = gs.getValue<bool>("autoconnect", false);

    this->communication.voiceEnabled = gs.getValue<bool>("comms-voice-enabled", true);
}