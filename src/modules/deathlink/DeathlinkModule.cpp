#include "DeathlinkModule.hpp"
#include <Geode/modify/PlayLayer.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>
#include <globed/core/RoomManager.hpp>
#include <globed/core/game/RemotePlayer.hpp>

using namespace geode::prelude;

namespace globed {

DeathlinkModule::DeathlinkModule() {}

void DeathlinkModule::onModuleInit()
{
    this->setAutoEnableMode(AutoEnableMode::Level);
}

void DeathlinkModule::onJoinLevel(GlobedGJBGL *gjbgl, GJGameLevel *level, bool editor)
{
    // if deathlink is disabled, disable the module for this level
    if (!RoomManager::get().getSettings().deathlink) {
        (void)this->disable();
    }
}

void DeathlinkModule::onPlayerDeath(GlobedGJBGL *gjbgl, RemotePlayer *player, const PlayerDeath &death)
{
    if (!death.isReal || !player || !player->isTeammate())
        return;

    gjbgl->killLocalPlayer();
}

struct GLOBED_MODIFY_ATTR DLPlayLayer : geode::Modify<DLPlayLayer, PlayLayer> {
    static void onModify(auto &self)
    {
        (void)self.setHookPriority("PlayLayer::resetLevel", -999);

        GLOBED_CLAIM_HOOKS(DeathlinkModule::get(), self, "PlayLayer::resetLevel", );
    }

    $override void resetLevel()
    {
        auto gameLayer = GlobedGJBGL::get();

        if (gameLayer && gameLayer->active() && gameLayer->isManuallyResetting()) {
            // if the user hit R or otherwise manually reset the level, just play it as a normal death instead
            gameLayer->killLocalPlayer();
            return;
        }

        PlayLayer::resetLevel();
    }
};

} // namespace globed
