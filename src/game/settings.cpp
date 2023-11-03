#include "settings.hpp"

GLOBED_SINGLETON_DEF(GlobedSettings)
GlobedSettings::GlobedSettings() {}

CachedSettings GlobedSettings::getCached() {
    return CachedSettings {
        .test = get<bool>("test")
    };
}