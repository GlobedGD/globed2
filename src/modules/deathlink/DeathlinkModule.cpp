#include "DeathlinkModule.hpp"
#include <globed/core/game/RemotePlayer.hpp>
#include <globed/core/RoomManager.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>

using namespace geode::prelude;

namespace globed {

DeathlinkModule::DeathlinkModule() {}

void DeathlinkModule::onModuleInit() {
    log::info("");
    this->setAutoEnableMode(AutoEnableMode::Level);
}

void DeathlinkModule::onJoinLevel(GlobedGJBGL* gjbgl, GJGameLevel* level, bool editor) {
    // if deathlink is disabled, disable the module for this level
    if (!RoomManager::get().getSettings().deathlink) {
        (void) this->disable();
    }
}

void DeathlinkModule::onPlayerDeath(GlobedGJBGL* gjbgl, RemotePlayer* player, const PlayerDeath& death) {
    if (!death.isReal || !player || !player->isTeammate()) return;

    gjbgl->killLocalPlayer();
}

}
