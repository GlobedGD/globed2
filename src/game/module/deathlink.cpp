#include "deathlink.hpp"

#include <hooks/gjbasegamelayer.hpp>
#include <hooks/play_layer.hpp>
#include <util/gd.hpp>

using util::gd::GameVariable;
using EventOutcome = BaseGameplayModule::EventOutcome;

EventOutcome DeathlinkModule::fullResetLevel() {
    // if the user hit R or otherwise manually reset the level, count as a death with deathlink
    if (gameLayer->m_fields->isManuallyResettingLevel) {
        this->notifyDeath();
    }

    return EventOutcome::Continue;
}

EventOutcome DeathlinkModule::resetLevel() {
    // if the user hit R or otherwise manually reset the level, count as a death with deathlink
    if (gameLayer->m_fields->isManuallyResettingLevel) {
        this->notifyDeath();
    }

    return EventOutcome::Continue;
}

EventOutcome DeathlinkModule::destroyPlayerPre(PlayerObject* player, GameObject* object) {
    // if deathlink is enabled, toggle faster reset off
    this->oldFastReset = util::gd::variable(GameVariable::FastRespawn);
    util::gd::setVariable(GameVariable::FastRespawn, false);

    return EventOutcome::Continue;
}

void DeathlinkModule::destroyPlayerPost(PlayerObject* player, GameObject* object) {
    util::gd::setVariable(GameVariable::FastRespawn, this->oldFastReset);
}

void DeathlinkModule::onUpdatePlayer(int playerId, RemotePlayer* player, const FrameFlags& flags) {
    if (flags.pendingRealDeath && !this->hasBeenKilled) {
        this->hasBeenKilled = true;

        auto& fields = gameLayer->m_fields;

        // force a fake death
        fields->isFakingDeath = true;

        if (!gameLayer->isEditor()) {
            this->getPlayLayer()->PlayLayer::destroyPlayer(gameLayer->m_player1, nullptr);
        }

        fields->isFakingDeath = false;
    }
}

void DeathlinkModule::selUpdate(float dt) {
    // reset the bool
    this->hasBeenKilled = false;
}

void DeathlinkModule::playerDestroyed(PlayerObject* player, bool unk) {
    this->notifyDeath();
}

void DeathlinkModule::notifyDeath() {
    auto& fields = gameLayer->m_fields;

    fields->lastDeathTimestamp = fields->timeCounter;
    fields->isLastDeathReal = !fields->isFakingDeath;
}