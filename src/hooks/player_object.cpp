#include "player_object.hpp"

#include <util/debugging.hpp>

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

    log::debug("playing death effect: {}", deathEffect);

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