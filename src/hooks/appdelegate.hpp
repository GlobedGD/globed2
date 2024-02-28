#pragma once
#include <defs/geode.hpp>
#include <Geode/modify/AppDelegate.hpp>

class $modify(GlobedAppDelegate, AppDelegate) {
    $override
    void applicationDidEnterBackground();

    $override
    void applicationWillEnterForeground();

#ifdef GEODE_IS_MACOS
    $override
    void loadingIsFinished();
#endif
};
