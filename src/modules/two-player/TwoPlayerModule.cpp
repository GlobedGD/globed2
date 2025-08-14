#include "TwoPlayerModule.hpp"
#include <globed/core/game/RemotePlayer.hpp>
#include <globed/core/RoomManager.hpp>
#include <globed/core/PopupManager.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>
#include <core/net/NetworkManagerImpl.hpp>

using namespace geode::prelude;

namespace globed {

TwoPlayerModule::TwoPlayerModule() {}

void TwoPlayerModule::onModuleInit() {
    this->setAutoEnableMode(AutoEnableMode::Level);
}

void TwoPlayerModule::sendLinkEvent(int player, bool player1) {
    Event event{};
    event.type = EVENT_2P_LINK_REQUEST;

    qn::HeapByteWriter writer;
    writer.writeI32(player);
    writer.writeBool(player1);
    event.data = std::move(writer).intoVector();

    NetworkManagerImpl::get().queueGameEvent(std::move(event));
}


void TwoPlayerModule::onJoinLevel(GlobedGJBGL* gjbgl, GJGameLevel* level, bool editor) {
    // if 2p mode is disabled, disable the module for this level
    if (!RoomManager::get().getSettings().twoPlayerMode) {
        (void) this->disable();
    }
}

void TwoPlayerModule::onPlayerDeath(GlobedGJBGL* gjbgl, RemotePlayer* player, const PlayerDeath& death) {
    if (death.isReal && player && player->isTeammate(false)) {
        return;
    }
}

}
