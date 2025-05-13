#include "appdelegate.hpp"

#include <net/manager.hpp>

using namespace geode::prelude;

#ifdef GEODE_IS_MOBILE
void GlobedAppDelegate::applicationDidEnterBackground() {
    log::debug("App going to background, suspending networking");

    NetworkManager::get().suspend();

    AppDelegate::applicationDidEnterBackground();
}

void GlobedAppDelegate::applicationWillEnterForeground() {
    log::debug("App going back to foreground, resuming networking");

    auto& nm = NetworkManager::get();
    nm.resume();

    AppDelegate::applicationWillEnterForeground();
}
#endif // GEODE_IS_MOBILE

