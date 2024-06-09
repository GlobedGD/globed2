#include "appdelegate.hpp"

#include <net/manager.hpp>

using namespace geode::prelude;

#ifdef GEODE_IS_ANDROID
void GlobedAppDelegate::applicationDidEnterBackground() {
    NetworkManager::get().suspend();

    AppDelegate::applicationDidEnterBackground();
}

void GlobedAppDelegate::applicationWillEnterForeground() {
    auto& nm = NetworkManager::get();
    nm.resume();

    AppDelegate::applicationWillEnterForeground();
}
#endif // GEODE_IS_ANDROID
