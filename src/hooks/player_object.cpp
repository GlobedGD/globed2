#include "player_object.hpp"

static inline void* const MAGIC_CONSTANT = (void*)0xdc00cd00;

void ComplexPlayerObject::setRemoteState() {
    this->setUserData(MAGIC_CONSTANT);
}

bool ComplexPlayerObject::vanilla() {
    return this->getUserData() != MAGIC_CONSTANT;
}

void ComplexPlayerObject::incrementJumps() {
    if (vanilla()) PlayerObject::incrementJumps();
}