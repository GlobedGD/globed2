#include <modules/two-player/TwoPlayerModule.hpp>
#include <globed/config.hpp>
#include <globed/core/game/VisualPlayer.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>

using namespace geode::prelude;

namespace globed {

struct GLOBED_MODIFY_ATTR TPPlayerObject : geode::Modify<TPPlayerObject, PlayerObject> {
    static void onModify(auto& self) {
        GLOBED_CLAIM_HOOKS(TwoPlayerModule::get(), self,
            "PlayerObject::update"
        );
    }

    $override
    void update(float dt) {
        PlayerObject::update(dt);

        auto& mod = TwoPlayerModule::get();
        auto gameLayer = GlobedGJBGL::get();

        if (!gameLayer || !mod.isLinked()) return;

        PlayerObject* noclipFor = mod.isPlayer2() ? gameLayer->m_player1 : gameLayer->m_player2;

        if (this == noclipFor) {
            auto pobj = mod.getLinkedPlayerObject(!mod.isPlayer2());
            this->updateFromLinkedPlayer(pobj);

            this->setVisible(false);
            m_playEffects = false;

            if (m_regularTrail) m_regularTrail->setVisible(false);
            if (m_waveTrail) m_waveTrail->setVisible(false);
            if (m_ghostTrail) m_ghostTrail->setVisible(false);
            if (m_trailingParticles) m_trailingParticles->setVisible(false);
            if (m_shipStreak) m_shipStreak->setVisible(false);
            if (m_playerGroundParticles) m_playerGroundParticles->setVisible(false);
            if (m_vehicleGroundParticles) m_vehicleGroundParticles->setVisible(false);
        }
    }

    void updateFromLinkedPlayer(VisualPlayer* linked) {
        if (!linked) {
            log::warn("linked player not found! i am {}", this);
            return;
        }

        this->setPosition(linked->getLastPosition());
    }
};

}