#include "player_object.hpp"

#include <util/debug.hpp>
#include <util/lowlevel.hpp>
#include <Geode/Utils.hpp>

#if UINTPTR_MAX > 0xffffffff
static inline constexpr uintptr_t MAGIC_CONSTANT = 0xdc00cd00dc00cd00;
#else
static inline constexpr uintptr_t MAGIC_CONSTANT = 0xdc00cd00;
#endif

void ComplexPlayerObject::setRemoteState() {
    this->setUserData((void*)MAGIC_CONSTANT);

#ifdef GEODE_IS_WINDOWS
    constexpr size_t getPositionIdx = 25;
#elif defined(GEODE_IS_ANDROID)
    constexpr size_t getPositionIdx = 24;
#else
# error getPosition unimpl
#endif

    util::lowlevel::vmtHook(&ComplexPlayerObject::getPositionHook, this, getPositionIdx);
}

void ComplexPlayerObject::setDeathEffect(int deathEffect) {
    // there was no reason for me to do this
    log::debug("setting death effect to {}", deathEffect);
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
    PlayerObject::playDeathEffect();
    gm->setPlayerDeathEffect(oldEffect);
}

bool ComplexPlayerObject::vanilla() {
    return ((uintptr_t)this->getUserData() & ~0xff) != MAGIC_CONSTANT;
}

void ComplexPlayerObject::incrementJumps() {
    if (vanilla()) PlayerObject::incrementJumps();
}

void ComplexPlayerObject::setPosition(const cocos2d::CCPoint& p) {
    if (vanilla()) {
        return PlayerObject::setPosition(p);
    }

    // do nothing.
}

const cocos2d::CCPoint& ComplexPlayerObject::getPositionHook() {
    if (vanilla()) {
        return this->CCNode::getPosition();
    }

    auto parent = this->getParent();
    if (parent == nullptr) {
        return this->CCNode::getPosition();
    }

    return parent->getPosition();
}