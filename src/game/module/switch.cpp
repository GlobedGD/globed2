#include "switch.hpp"

#include <hooks/gjbasegamelayer.hpp>
#include <hooks/play_layer.hpp>
#include <util/gd.hpp>

using EventOutcome = BaseGameplayModule::EventOutcome;

SwitchModule::SwitchModule(GlobedGJBGL* gameLayer) : BaseGameplayModule(gameLayer) {}

EventOutcome SwitchModule::resetLevel() {
    auto pl = this->gameLayer;
    if (!pl) return EventOutcome::Continue;

    pl->m_fields->npTimeCounter = 0.f;

    return EventOutcome::Continue;
}

void SwitchModule::mainPlayerUpdate(PlayerObject* player, float dt) {
    if (!player->getUserObject(LOCKED_TO_KEY)) return;

    PlayerObject* noclipFor = gameLayer->m_player1;

    if (player == noclipFor && this->linkedTo != 0) {
        this->updateFromLockedPlayer(player);
        player->setVisible(false);
        // if (bgl->m_gameState.m_isDualMode) {
        //     this->m_isHidden = true;
        // }

        player->m_playEffects = false;
        if (player->m_regularTrail) player->m_regularTrail->setVisible(false);
        if (player->m_waveTrail) player->m_waveTrail->setVisible(false);
        if (player->m_ghostTrail) player->m_ghostTrail->setVisible(false);
        if (player->m_trailingParticles) player->m_trailingParticles->setVisible(false);
        if (player->m_shipStreak) player->m_shipStreak->setVisible(false);
    }
}

void SwitchModule::updateFromLockedPlayer(PlayerObject* player) {
    ComplexVisualPlayer* cvp = static_cast<ComplexVisualPlayer*>(player->getUserObject(LOCKED_TO_KEY));
    if (!cvp) return;

    RemotePlayer* rp = cvp->getRemotePlayer();

    if (rp->lastFrameFlags.pendingDeath) {
        Loader::get()->queueInMainThread([] {
            static_cast<GlobedPlayLayer*>(PlayLayer::get())->forceKill(PlayLayer::get()->m_player1);
        });
    }

    if (!rp->lastVisualState.isDead) {
        auto link = static_cast<PlayerObject*>(cvp->getPlayerObject());
        util::gd::clonePlayerObject(player, link);
    }
}

void SwitchModule::linkPlayerTo(int accountId) {
    log::debug("Link attempt to {}", accountId);
    if (!gameLayer->m_fields->players.contains(accountId)) return;

    RemotePlayer* rp = gameLayer->m_fields->players.at(accountId);

    log::debug("Linking to {}", rp->getAccountData().name);

    PlayerObject* ignored = gameLayer->m_player1;

    ignored->setUserObject(LOCKED_TO_KEY, rp->player1);

    this->linkedTo = accountId;
}

void SwitchModule::selUpdate(float dt) {
    auto pl = this->gameLayer;
    if (!pl) return;

    auto& fields = pl->getFields();

    if (fields.lastExecutedSwitch.timestamp != fields.nextSwitchData.timestamp) {
        if (fields.nextSwitchData.timestamp <= fields.npTimeCounter) {
            pl->executeSwitch(fields.nextSwitchData);
            fields.lastExecutedSwitch = fields.nextSwitchData;
        }
    }

    if (linkedTo != fields.lastExecutedSwitch.player) {
        if (fields.lastExecutedSwitch.player != GJAccountManager::get()->m_accountID) {
            this->linkPlayerTo(fields.lastExecutedSwitch.player);
        } else {
            this->unlink();
        }
    }
}

void SwitchModule::unlink() {
    this->linkedTo = 0;
    gameLayer->m_player1->setUserObject(LOCKED_TO_KEY, nullptr);
}
