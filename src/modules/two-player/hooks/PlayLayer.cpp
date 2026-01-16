#include <core/hooks/GJBaseGameLayer.hpp>
#include <globed/config.hpp>
#include <modules/two-player/TwoPlayerModule.hpp>

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

namespace globed {

struct GLOBED_MODIFY_ATTR TPPlayLayer : geode::Modify<TPPlayLayer, PlayLayer> {
    static void onModify(auto &self)
    {
        (void)self.setHookPriority("PlayLayer::destroyPlayer", -9999);
        (void)self.setHookPriority("PlayLayer::resetLevel", -999);

        GLOBED_CLAIM_HOOKS(TwoPlayerModule::get(), self, "PlayLayer::destroyPlayer", "PlayLayer::resetLevel", );
    }

    $override void destroyPlayer(PlayerObject *player, GameObject *obj)
    {
        auto &mod = TwoPlayerModule::get();
        auto gameLayer = GlobedGJBGL::get();

        if (!gameLayer || !mod.isLinked() || mod.ignoreNoclip()) {
            PlayLayer::destroyPlayer(player, obj);
            return;
        }

        PlayerObject *noclipFor = mod.isPlayer2() ? gameLayer->m_player1 : gameLayer->m_player2;
        if (obj != m_anticheatSpike && player == noclipFor) {
            return;
        }

        PlayLayer::destroyPlayer(player, obj);
    }

    $override void resetLevel()
    {
        auto &mod = TwoPlayerModule::get();
        auto gameLayer = GlobedGJBGL::get();

        if (mod.isLinked() && gameLayer && gameLayer->active() && gameLayer->isManuallyResetting()) {
            // if the user hit R or otherwise manually reset the level, just play it as a normal death instead
            mod.causeLocalDeath(gameLayer);
            return;
        }

        PlayLayer::resetLevel();
    }
};

} // namespace globed