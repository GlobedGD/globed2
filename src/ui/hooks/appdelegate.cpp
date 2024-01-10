#include "appdelegate.hpp"
#include <net/network_manager.hpp>

void GlobedAppDelegate::applicationDidEnterBackground() {
    NetworkManager::get().suspend();
    AppDelegate::applicationDidEnterBackground();
}

void GlobedAppDelegate::applicationWillEnterForeground() {
    NetworkManager::get().resume();
    AppDelegate::applicationWillEnterForeground();
}