#include "APSModule.hpp"
#include "Hooks.hpp"
#include <core/hooks/GJBaseGameLayer.hpp>

using namespace geode::prelude;

namespace globed {

APSModule::APSModule() {}

void APSModule::onModuleInit() {
    this->setAutoEnableMode(AutoEnableMode::Level);
}

void APSModule::onPlayerDeath(GlobedGJBGL* gjbgl, RemotePlayer* player, const PlayerDeath& death) {
    if (auto pl = APSPlayLayer::get(gjbgl)) {
        pl->handlePlayerDeath(death, player);
    }
}

}
