#include "appdelegate.hpp"
#include <net/network_manager.hpp>

void GlobedAppDelegate::applicationDidEnterBackground() {
#ifdef GEODE_IS_ANDROID
    NetworkManager::get().suspend();
#endif

    AppDelegate::applicationDidEnterBackground();
}

void GlobedAppDelegate::applicationWillEnterForeground() {
#ifdef GEODE_IS_ANDROID
    NetworkManager::get().resume();
#endif

    AppDelegate::applicationWillEnterForeground();
}