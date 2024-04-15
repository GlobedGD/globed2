#include "player_object.hpp"

#include <hooks/gjbasegamelayer.hpp>
#include <util/ui.hpp>
#include <util/debug.hpp>

using namespace geode::prelude;

void ComplexPlayerObject::setRemotePlayer(ComplexVisualPlayer* rp) {
    this->setUserObject(rp);

    // also set remote state
    this->setTag(COMPLEX_PLAYER_OBJECT_TAG);
}

bool ComplexPlayerObject::vanilla() {
    return this->getTag() != COMPLEX_PLAYER_OBJECT_TAG;
}

void ComplexPlayerObject::playDeathEffect() {
    if (vanilla()) {
        PlayerObject::playDeathEffect();
        return;
    }

    auto* rp = static_cast<ComplexVisualPlayer*>(this->getUserObject());
    int deathEffect = rp->storedIcons.deathEffect;

    auto* gm = GameManager::get();

    // we need to do this because the orig func reads the death effect ID from GameManager
    int oldEffect = gm->getPlayerDeathEffect();
    gm->setPlayerDeathEffect(deathEffect);

    // also make sure the death effect is properly loaded
    util::ui::tryLoadDeathEffect(deathEffect);

    PlayerObject::playDeathEffect();
    gm->setPlayerDeathEffect(oldEffect);
}

void ComplexPlayerObject::incrementJumps() {
    if (vanilla()) PlayerObject::incrementJumps();
}


void HookedPlayerObject::playSpiderDashEffect(cocos2d::CCPoint from, cocos2d::CCPoint to) {
    // if we are in the editor, do nothing
    if (GJBaseGameLayer::get() == nullptr) {
        PlayerObject::playSpiderDashEffect(from, to);
        return;
    }

    auto* gpl = GlobedGJBGL::get();
    if (this == gpl->m_player1) {
        gpl->m_fields->spiderTp1 = SpiderTeleportData { .from = from, .to = to };
    } else if (this == gpl->m_player2) {
        gpl->m_fields->spiderTp2 = SpiderTeleportData { .from = from, .to = to };
    }

    PlayerObject::playSpiderDashEffect(from, to);
}

void HookedPlayerObject::incrementJumps() {
    if (GJBaseGameLayer::get() == nullptr || m_isOnSlope) {
        PlayerObject::incrementJumps();
        return;
    }

    auto* gpl = GlobedGJBGL::get();

    if (this == gpl->m_player1) {
        gpl->m_fields->didJustJumpp1 = true;
    } else if (this == gpl->m_player2) {
        gpl->m_fields->didJustJumpp2 = true;
    }

    PlayerObject::incrementJumps();
}

void HookedPlayerObject::update(float dt) {
    auto* gjbgl = GJBaseGameLayer::get();
    if (!gjbgl) {
        PlayerObject::update(dt);
        return;
    }

    auto* gpl = GlobedGJBGL::get();
    if (gpl->m_fields->forcedPlatformer) {
        this->togglePlatformerMode(true);

#ifdef GEODE_IS_ANDROID
        if (!m_fields->forcedPlatFlag) {
            if (gpl->m_uiLayer) {
                m_fields->forcedPlatFlag = true;
                gpl->m_uiLayer->togglePlatformerMode(true);
            }
        }
#endif
    }

    PlayerObject::update(dt);
}
