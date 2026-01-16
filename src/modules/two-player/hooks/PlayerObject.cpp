#include <core/hooks/GJBaseGameLayer.hpp>
#include <globed/config.hpp>
#include <globed/core/game/VisualPlayer.hpp>
#include <modules/two-player/TwoPlayerModule.hpp>

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>

using namespace geode::prelude;

namespace globed {

struct GLOBED_MODIFY_ATTR TPPlayerObject : geode::Modify<TPPlayerObject, PlayerObject> {
    static void onModify(auto &self)
    {
        GLOBED_CLAIM_HOOKS(TwoPlayerModule::get(), self, "PlayerObject::update");
    }

    $override void update(float dt)
    {
        PlayerObject::update(dt);

        auto &mod = TwoPlayerModule::get();
        auto gameLayer = GlobedGJBGL::get();

        if (!gameLayer || !mod.isLinked())
            return;

        PlayerObject *noclipFor = mod.isPlayer2() ? gameLayer->m_player1 : gameLayer->m_player2;

        if (this == noclipFor) {
            auto pobj = mod.getLinkedPlayerObject(!mod.isPlayer2());
            this->updateFromLinkedPlayer(pobj);

            this->setVisible(false);
            m_playEffects = false;

            if (m_regularTrail)
                m_regularTrail->setVisible(false);
            if (m_waveTrail)
                m_waveTrail->setVisible(false);
            if (m_ghostTrail)
                m_ghostTrail->setVisible(false);
            if (m_trailingParticles)
                m_trailingParticles->setVisible(false);
            if (m_shipStreak)
                m_shipStreak->setVisible(false);
            if (m_playerGroundParticles)
                m_playerGroundParticles->setVisible(false);
            if (m_vehicleGroundParticles)
                m_vehicleGroundParticles->setVisible(false);
        }
    }

    void updateFromLinkedPlayer(VisualPlayer *linked)
    {
        if (!linked) {
            log::warn("linked player not found! i am {}", this);
            return;
        }

        bool shouldUpdateArt = (m_gravity != linked->m_gravity) || (m_isUpsideDown != linked->m_isUpsideDown) ||
                               m_isGoingLeft != linked->m_isGoingLeft;

        auto pos = linked->getLastPosition();
        this->m_position = pos;
        this->m_yVelocity = linked->m_yVelocity;
        this->m_platformerXVelocity = linked->m_platformerXVelocity;
        this->m_isOnGround = linked->m_isOnGround;
        this->m_isGoingLeft = linked->m_isGoingLeft;
        this->m_isAccelerating = linked->m_isAccelerating;
        this->m_accelerationOrSpeed = linked->m_accelerationOrSpeed;
        this->m_fallStartY = linked->m_fallStartY;
        this->m_gravity = linked->m_gravity;
        this->m_gravityMod = linked->m_gravityMod;
        this->m_isOnGround2 = linked->m_isOnGround2;
        this->m_touchedPad = linked->m_touchedPad;
        this->m_isUpsideDown = linked->m_isUpsideDown;
        this->setPosition(pos);

        // TODO: doesnt work
        // if (shouldUpdateArt) {
        //     this->updatePlayerArt();
        // }
    }
};

} // namespace globed