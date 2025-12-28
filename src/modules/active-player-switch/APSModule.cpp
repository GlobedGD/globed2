#include "APSModule.hpp"
#include "Hooks.hpp"
#include <globed/core/RoomManager.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>

using namespace geode::prelude;

namespace globed {

APSModule::APSModule() {}

void APSModule::onModuleInit() {
    this->setAutoEnableMode(AutoEnableMode::Level);
}

void APSModule::onJoinLevel(GlobedGJBGL* gjbgl, GJGameLevel* level, bool editor) {
    if (!RoomManager::get().getSettings().switcheroo) {
        (void) this->disable();
    }
}

void APSModule::onPlayerDeath(GlobedGJBGL* gjbgl, RemotePlayer* player, const PlayerDeath& death) {
    if (auto pl = APSPlayLayer::get(gjbgl)) {
        pl->handlePlayerDeath(death, player);
    }
}

}
