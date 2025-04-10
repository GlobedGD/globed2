#include "collision.hpp"
#include <hooks/gjbasegamelayer.hpp>

using namespace geode::prelude;

void CollisionModule::loadLevelSettingsPre() {
    LevelSettingsObject* lo = gameLayer->m_levelSettings;

    auto* gpl = GlobedGJBGL::get();

    this->lastPlat = lo->m_platformerMode;
    this->lastLength = gameLayer->m_level->m_levelLength;

    if (gpl->m_fields->forcedPlatformer) {
        lo->m_platformerMode = true;
    }
}

void CollisionModule::loadLevelSettingsPost() {
    LevelSettingsObject* lo = gameLayer->m_levelSettings;

    lo->m_platformerMode = this->lastPlat;
    gameLayer->m_level->m_levelLength = this->lastLength;
}

static bool shouldCorrectCollision(const CCRect& p1, const CCRect& p2, CCPoint& displacement) {
    if (std::abs(displacement.x) > 10.f) {
        displacement.x = -displacement.x;
        displacement.y = 0;
        return true;
    }

    return false;
}

void CollisionModule::checkCollisions(PlayerObject* player, float dt, bool p2) {
    bool isSecond = player == gameLayer->m_player2;

    for (const auto& [_, rp] : gameLayer->m_fields->players) {
        auto* p1 = rp->player1->getPlayerObject();
        auto* p2 = rp->player2->getPlayerObject();

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
}

bool CollisionModule::shouldSaveProgress() {
    return false;
}
