#include "CollisionModule.hpp"
#include <globed/core/game/RemotePlayer.hpp>
#include <globed/core/RoomManager.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>

#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

namespace globed {

CollisionModule::CollisionModule() {}

void CollisionModule::onModuleInit() {
    this->setAutoEnableMode(AutoEnableMode::Level);
}

void CollisionModule::onJoinLevel(GlobedGJBGL* gjbgl, GJGameLevel* level, bool editor) {
    // if deathlink is disabled, disable the module for this level
    if (!RoomManager::get().getSettings().collision) {
        (void) this->disable();
    } else {
        // safe mode is forced in collision mode
        gjbgl->setPermanentSafeMode();
    }
}

static bool shouldCorrectCollision(const CCRect& p1, const CCRect& p2, CCPoint& displacement) {
    if (std::abs(displacement.x) > 10.f) {
        displacement.x = -displacement.x;
        displacement.y = 0;
        return true;
    }

    return false;
}

void CollisionModule::checkCollisions(GlobedGJBGL* gjbgl, PlayerObject* player, float dt, bool p2) {
    bool isSecond = player == gjbgl->m_player2;

    for (const auto& [_, rp] : gjbgl->m_fields->m_players) {
        auto* p1 = rp->player1();
        auto* p2 = rp->player2();

        auto& p1Rect = p1->getObjectRect();
        auto& p2Rect = p2->getObjectRect();

        auto& playerRect = player->getObjectRect();

        CCRect p1CollRect = p1Rect;
        CCRect p2CollRect = p2Rect;

        constexpr float padding = 2.f;

        // p1CollRect.origin += CCPoint{padding, padding};
        // p1CollRect.size -= CCSize{padding * 2, padding * 2};

        // p2CollRect.origin += CCPoint{padding, padding};
        // p2CollRect.size -= CCSize{padding * 2, padding * 2};

        bool intersectsP1 = playerRect.intersectsRect(p1CollRect);
        bool intersectsP2 = playerRect.intersectsRect(p2CollRect);

        if (isSecond) {
            rp->player1()->setStickyState(false, false);
            rp->player2()->setStickyState(false, false);
        } else {
            rp->player1()->setStickyState(true, false);
            rp->player2()->setStickyState(true, false);
        }

        if (intersectsP1) {
            auto prev = player->getPosition();
            player->collidedWithObject(dt, p1, p1CollRect, false);
            auto displacement = player->getPosition() - prev;

            // log::debug("p1 intersect, displacement: {}", displacement);

            bool shouldRevert = shouldCorrectCollision(playerRect, p1Rect, displacement);

            if (shouldRevert) {
                player->setPosition(player->getPosition() + displacement);
            }

            if (std::abs(displacement.y) > 0.001f) {
                rp->player1()->setStickyState(!isSecond, true);
            }
        }

        if (intersectsP2) {
            auto prev = player->getPosition();
            player->collidedWithObject(dt, p2, p2CollRect, false);
            auto displacement = player->getPosition() - prev;

            // log::debug("p2 intersect, displacement: {}", displacement);

            bool shouldRevert = shouldCorrectCollision(playerRect, p2Rect, displacement);

            if (shouldRevert) {
                player->setPosition(player->getPosition() + displacement);
            }

            if (std::abs(displacement.y) > 0.001f) {
                rp->player2()->setStickyState(!isSecond, true);
            }
        }
    }
}

struct GLOBED_MODIFY_ATTR CollisionGJBGL : Modify<CollisionGJBGL, GJBaseGameLayer> {
    int checkCollisions(PlayerObject* player, float dt, bool p2) {
        int retval = GJBaseGameLayer::checkCollisions(player, dt, p2);

        auto gjbgl = GlobedGJBGL::get(this);
        if (!gjbgl->active()) {
            return retval;
        }

        CollisionModule::get().checkCollisions(gjbgl, player, dt, p2);

        return retval;
    }
};

}
