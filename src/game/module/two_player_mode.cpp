#include "two_player_mode.hpp"

#include <managers/room.hpp>
#include <managers/popup.hpp>
#include <hooks/gjbasegamelayer.hpp>
#include <hooks/play_layer.hpp>
#include <ui/game/userlist/user_cell.hpp>
#include <util/gd.hpp>

using EventOutcome = BaseGameplayModule::EventOutcome;
using UserCellButton = BaseGameplayModule::UserCellButton;
using util::gd::GameVariable;
using namespace geode::prelude;

TwoPlayerModeModule::TwoPlayerModeModule(GlobedGJBGL* gameLayer) : BaseGameplayModule(gameLayer) {
    this->isPrimary = RoomManager::get().isOwner();
}

void TwoPlayerModeModule::mainPlayerUpdate(PlayerObject* player, float dt) {
    if (!player->getUserObject(LOCKED_TO_KEY)) return;

    PlayerObject* noclipFor = this->isPrimary ? gameLayer->m_player2 : gameLayer->m_player1;

    if (player == noclipFor && this->linked) {
        // this->updateFromLockedPlayer(player, !isPrimary && gameLayer->m_gameState.m_isDualMode);
        this->updateFromLockedPlayer(player, false);
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
        if (player->m_playerGroundParticles) player->m_playerGroundParticles->setVisible(false);
        if (player->m_vehicleGroundParticles) player->m_vehicleGroundParticles->setVisible(false);

        this->gameLayer->moveCameraToPos(player->getPosition());
    }
}

void TwoPlayerModeModule::updateFromLockedPlayer(PlayerObject* player, bool ignorePos) {
    ComplexVisualPlayer* cvp = static_cast<ComplexVisualPlayer*>(player->getUserObject(LOCKED_TO_KEY));
    if (!cvp) return;

    RemotePlayer* rp = cvp->getRemotePlayer();

    if (rp->lastFrameFlags.pendingDeath) {
        Loader::get()->queueInMainThread([] {
            auto pl = globed::cachedSingleton<GameManager>()->m_playLayer;
            static_cast<GlobedPlayLayer*>(pl)->forceKill(pl->m_player1);
        });
    }

    if (!ignorePos && !rp->lastVisualState.isDead) {
        player->setPosition(cvp->getPlayerPosition());
    }
}

EventOutcome TwoPlayerModeModule::resetLevel() {
    auto pl = this->getPlayLayer();
    if (!pl || !this->linked) return EventOutcome::Continue;

    if (gameLayer->m_fields->setupWasCompleted && gameLayer->m_fields->isManuallyResettingLevel) {
        pl->forceKill(isPrimary ? pl->m_player1 : pl->m_player2);
        return EventOutcome::Halt;
    }

    return EventOutcome::Continue;
}

EventOutcome TwoPlayerModeModule::destroyPlayerPre(PlayerObject* player, GameObject* object) {
    auto pl = this->getPlayLayer();

    if (!pl->m_fields->ignoreNoclip && this->linked) {
        PlayerObject* noclipFor = this->isPrimary ? gameLayer->m_player2 : gameLayer->m_player1;

        if (pl->m_fields->antiCheat != object && player == noclipFor) {
            // noclip
            // log::debug("noclipping for {}", noclipFor == m_player1 ? "player 1" : "player 2");
            return EventOutcome::Halt;
        }
    }

    // if deathlink or 2p mode is enabled, toggle faster reset depending on room setting
    this->oldFastReset = util::gd::variable(GameVariable::FastRespawn);
    util::gd::setVariable(GameVariable::FastRespawn, this->gameLayer->m_fields->roomSettings.fasterReset);

    return EventOutcome::Continue;
}

void TwoPlayerModeModule::destroyPlayerPost(PlayerObject* player, GameObject* object) {
    util::gd::setVariable(GameVariable::FastRespawn, this->oldFastReset);
}

void TwoPlayerModeModule::linkPlayerTo(int accountId) {
    log::debug("Link attempt to {}", accountId);
    if (accountId == 0 || !gameLayer->m_fields->players.contains(accountId)) {
        this->unlink();
        return;
    }

    RemotePlayer* rp = gameLayer->m_fields->players.at(accountId);

    log::debug("Linking to {}", rp->getAccountData().name);

    PlayerObject* ignored = this->isPrimary ? gameLayer->m_player2 : gameLayer->m_player1;

    ignored->setUserObject(LOCKED_TO_KEY, isPrimary ? rp->player2 : rp->player1);

    this->linked = true;
    this->linkedId = accountId;
}

int TwoPlayerModeModule::getLinkedTo() {
    return linkedId;
}

bool TwoPlayerModeModule::onUnpause() {
    this->unlinkIfAlone();

    if (!this->linked) {
        PopupManager::get().alert("Globed Error", "Cannot unpause while not linked to a player.").showInstant();
        return false;
    }

    return true;
}

void TwoPlayerModeModule::unlinkIfAlone() {
    if (!this->linked) return;

    auto& players = gameLayer->m_fields->players;
    if (!players.contains(linkedId)) {
        this->unlink();
        return;
    }
}

void TwoPlayerModeModule::unlink() {
    this->linked = false;
    this->linkedId = 0;

    gameLayer->m_player1->setUserObject(LOCKED_TO_KEY, nullptr);
    gameLayer->m_player2->setUserObject(LOCKED_TO_KEY, nullptr);
}

bool TwoPlayerModeModule::shouldSaveProgress() {
    // safe mode
    return false;
}

void TwoPlayerModeModule::selPeriodicalUpdate(float dt) {
    this->unlinkIfAlone();

    auto pl = globed::cachedSingleton<GameManager>()->m_playLayer;
    auto gjbgl = static_cast<GlobedGJBGL*>(static_cast<GJBaseGameLayer*>(pl));

    if (pl && !gjbgl->isPaused() && !this->linked) {
        // pause the game
        pl->pauseGame(false);
    }
}

void TwoPlayerModeModule::updateCameraPre(float dt) {
    auto pl = globed::cachedSingleton<GameManager>()->m_playLayer;
    auto gjbgl = static_cast<GlobedGJBGL*>(static_cast<GJBaseGameLayer*>(pl));
    auto& fields = gjbgl->getFields();

    if (!gjbgl || !linked || isPrimary || !gjbgl->m_gameState.m_isDualMode) {
        preservedPlayerPos = CCPoint{};
        return;
    }

    preservedPlayerPos = gjbgl->m_player1->getPosition();
    gjbgl->m_player1->setPosition({gjbgl->m_player2->getPositionX(), preservedPlayerPos.y});
}

void TwoPlayerModeModule::updateCameraPost(float dt) {
    auto pl = globed::cachedSingleton<GameManager>()->m_playLayer;
    auto gjbgl = static_cast<GlobedGJBGL*>(static_cast<GJBaseGameLayer*>(pl));

    if (!gjbgl || !linked || isPrimary || !gjbgl->m_gameState.m_isDualMode || preservedPlayerPos.isZero()) {
        return;
    }

    // restore player position
    gjbgl->m_player1->setPosition(preservedPlayerPos);
}

std::vector<UserCellButton> TwoPlayerModeModule::onUserActionsPopup(int accountId, bool self) {
    if (self) return {};

    if (accountId == linkedId) {
        return std::vector({UserCellButton {
            .spriteName = "gj_linkBtnOff_001.png",
            .id = "2p-unlink-btn"_spr,
            .callback = [this](CCObject* sender){
                this->linkPlayerTo(0);
                return true;
            }
        }});
    }

    return std::vector({UserCellButton {
        .spriteName = "gj_linkBtn_001.png",
        .id = "2p-link-btn"_spr,
        .callback = [this, accountId](CCObject* sender){
            this->linkPlayerTo(accountId);
            return true;
        }
    }});
}

