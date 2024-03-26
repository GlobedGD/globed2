#include "gjbasegamelayer.hpp"

#include "play_layer.hpp"
#include "game_manager.hpp"

using namespace geode::prelude;

static bool shouldCorrectCollision(const CCRect& p1, const CCRect& p2, CCPoint& displacement) {
    if (std::abs(displacement.x) > 10.f) {
        displacement.x = -displacement.x;
        displacement.y = 0;
        return true;
    }

    return false;
}

bool GlobedGJBGL::init() {
    if (!GJBaseGameLayer::init()) return false;

    // yeah so like

    auto* gm = static_cast<HookedGameManager*>(GameManager::get());
    gm->setLastSceneEnum();

    return true;
}

int GlobedGJBGL::checkCollisions(PlayerObject* player, float dt, bool p2) {
    int retval = GJBaseGameLayer::checkCollisions(player, dt, p2);

    if ((void*)this != PlayLayer::get()) return retval;

    // up and down hell yeah
    auto* gpl = static_cast<GlobedPlayLayer*>(static_cast<GJBaseGameLayer*>(this));

    if (!gpl->m_fields->globedReady) return retval;
    if (!gpl->m_fields->playerCollisionEnabled) return retval;

    bool isSecond = player == gpl->m_player2;

    for (const auto& [_, rp] : gpl->m_fields->players) {
        auto* p1 = static_cast<PlayerObject*>(rp->player1->getPlayerObject());
        auto* p2 = static_cast<PlayerObject*>(rp->player2->getPlayerObject());

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
            rp->player1->setP2StickyState(false);
            rp->player2->setP2StickyState(false);
        } else {
            rp->player1->setP1StickyState(false);
            rp->player2->setP1StickyState(false);
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
                isSecond ? rp->player1->setP2StickyState(true) : rp->player1->setP1StickyState(true);
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
                isSecond ? rp->player2->setP2StickyState(true) : rp->player2->setP1StickyState(true);
            }
        }
    }

    return retval;
}