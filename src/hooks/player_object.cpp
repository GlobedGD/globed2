#include "player_object.hpp"

#include <hooks/play_layer.hpp>
#include <util/ui.hpp>

#if UINTPTR_MAX > 0xffffffff
static inline constexpr uintptr_t MAGIC_CONSTANT = 0xdc00cd00dc00cd00;
#else
static inline constexpr uintptr_t MAGIC_CONSTANT = 0xdc00cd00;
#endif

void ComplexPlayerObject::setRemoteState() {
    this->setUserData((void*)MAGIC_CONSTANT);
}

void ComplexPlayerObject::setDeathEffect(int deathEffect) {
    // there was no reason for me to do this
    this->setUserData((void*)(MAGIC_CONSTANT | static_cast<unsigned char>(deathEffect)));
}

void ComplexPlayerObject::playDeathEffect() {
    if (vanilla()) {
        PlayerObject::playDeathEffect();
        return;
    }

    int deathEffect = static_cast<int>(static_cast<unsigned char>((uintptr_t)this->getUserData() & 0xff));
    auto* gm = GameManager::get();

    // we need to do this because the orig func reads the death effect ID from GameManager
    int oldEffect = gm->getPlayerDeathEffect();
    gm->setPlayerDeathEffect(deathEffect);

    // also make sure the death effect is properly loaded
    util::ui::tryLoadDeathEffect(deathEffect);

    PlayerObject::playDeathEffect();
    gm->setPlayerDeathEffect(oldEffect);
}

bool ComplexPlayerObject::vanilla() {
    return ((uintptr_t)this->getUserData() & ~0xff) != MAGIC_CONSTANT;
}

void ComplexPlayerObject::incrementJumps() {
    if (vanilla()) PlayerObject::incrementJumps();
}


void HookedPlayerObject::playSpiderDashEffect(cocos2d::CCPoint from, cocos2d::CCPoint to) {
    // if we are in the editor, do nothing
    if (PlayLayer::get() == nullptr) {
        PlayerObject::playSpiderDashEffect(from, to);
        return;
    }

    auto* gpl = static_cast<GlobedPlayLayer*>(PlayLayer::get());
    if (this == gpl->m_player1) {
        gpl->m_fields->spiderTp1 = SpiderTeleportData { .from = from, .to = to };
    } else if (this == gpl->m_player2) {
        gpl->m_fields->spiderTp2 = SpiderTeleportData { .from = from, .to = to };
    }

    PlayerObject::playSpiderDashEffect(from, to);
}

void HookedPlayerObject::incrementJumps() {
    if (PlayLayer::get() == nullptr) {
        PlayerObject::incrementJumps();
        return;
    }

    auto* gpl = static_cast<GlobedPlayLayer*>(PlayLayer::get());

    if (this == gpl->m_player1) {
        gpl->m_fields->didJustJumpp1 = true;
    } else if (this == gpl->m_player2) {
        gpl->m_fields->didJustJumpp2 = true;
    }

    PlayerObject::incrementJumps();
}
