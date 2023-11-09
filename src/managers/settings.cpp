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