#pragma once
#include <defs.hpp>
#include <Geode/modify/AppDelegate.hpp>

class $modify(GlobedAppDelegate, AppDelegate) {
    void applicationDidEnterBackground();
    void applicationWillEnterForeground();
};
