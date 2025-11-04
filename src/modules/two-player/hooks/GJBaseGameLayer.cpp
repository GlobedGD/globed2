#include <modules/two-player/TwoPlayerModule.hpp>
#include <globed/config.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>

#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

using namespace geode::prelude;

namespace globed {

struct GLOBED_MODIFY_ATTR TPMBaseGameLayer : geode::Modify<TPMBaseGameLayer, GJBaseGameLayer> {
    static void onModify(auto& self) {
        GLOBED_CLAIM_HOOKS(TwoPlayerModule::get(), self,
            "GJBaseGameLayer::updateCamera",
            "GJBaseGameLayer::update",
        );
    }

    $override
    void updateCamera(float dt) {
        auto& mod = TwoPlayerModule::get();
        auto& lerper = GlobedGJBGL::get(this)->m_fields->m_interpolator;

        lerper.setCameraCorrections(!mod.isPlayer2());

        bool fixCamera = m_gameState.m_isDualMode && mod.isLinked() && mod.isPlayer2();
        if (!fixCamera) {
            GJBaseGameLayer::updateCamera(dt);
            return;
        }

        auto origPos = m_player1->getPosition();
        m_player1->setPosition({m_player2->getPositionX(), origPos.y});
        GJBaseGameLayer::updateCamera(dt);
        m_player1->setPosition(origPos);
    }

    $override
    void update(float dt) {
        GJBaseGameLayer::update(dt);

        auto& mod = TwoPlayerModule::get();
        if (!mod.isLinked()) {
            auto pl = PlayLayer::get();
            if (pl && GlobedGJBGL::get(pl)->active() && !pl->m_isPaused) {
                log::debug("force pausing game");
                pl->pauseGame(false);
            }
        }
    }
};

}