#pragma once
#include <defs/geode.hpp>

#include <Geode/modify/AppDelegate.hpp>

#include <util/time.hpp>

class $modify(GlobedAppDelegate, AppDelegate) {
#ifdef GEODE_IS_ANDROID
    $override
    void applicationDidEnterBackground();

    $override
    void applicationWillEnterForeground();
#endif // GEODE_IS_ANDROID

#ifdef GEODE_IS_MACOS
    $override
    void loadingIsFinished();
#endif
};
