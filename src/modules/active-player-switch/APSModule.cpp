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
    } else {
        // force safe mode
        log::info("Switcheroo - enabling safe mode for the level");
        gjbgl->setPermanentSafeMode();

        auto& lerper = gjbgl->m_fields->m_interpolator;
        gjbgl->toggleCullingEnabled(false);
        gjbgl->toggleExtendedData(true);
        lerper.setLowLatencyMode(true);
        lerper.setCameraCorrections(false);
    }
}

void APSModule::onPlayerDeath(GlobedGJBGL* gjbgl, RemotePlayer* player, const PlayerDeath& death) {
    if (auto pl = APSPlayLayer::get(gjbgl)) {
        pl->handlePlayerDeath(death, player);
    }
}

void APSModule::onPreUpdate(GlobedGJBGL* gjbgl, float dt) {
    if (auto pl = APSPlayLayer::get(gjbgl)) {
        pl->handleUpdate(dt);
    }
}

void APSModule::onLocalPlayerDeath(GlobedGJBGL* gjbgl, bool real) {
    // don't respawn immediately if we are not the main player, instead wait for the main player to respawn us
    auto apl = APSPlayLayer::get(gjbgl);
    auto& cont = apl->m_fields->m_controller;
    if (cont.m_gameActive && cont.m_activePlayer != 0 && !cont.m_meActive) {
        gjbgl->cancelLocalRespawn();
    }
}

void APSModule::onPlayerRespawn(GlobedGJBGL* gjbgl, RemotePlayer* player) {
    auto apl = APSPlayLayer::get(gjbgl);
    auto& cont = apl->m_fields->m_controller;

    // if the active player respawned, we respawn too
    if (cont.m_gameActive && !cont.m_meActive && cont.m_activePlayer == player->id()) {
        gjbgl->causeLocalRespawn();
    }
}

}
