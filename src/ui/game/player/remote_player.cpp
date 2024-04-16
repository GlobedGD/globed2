#include "remote_player.hpp"

#include <managers/settings.hpp>

using namespace geode::prelude;

bool RemotePlayer::init(PlayerProgressIcon* progressIcon, PlayerProgressArrow* progressArrow, const PlayerAccountData& data) {
    if (!CCNode::init()) return false;
    this->accountData = data;
    this->progressIcon = progressIcon;
    this->progressArrow = progressArrow;

    this->player1 = Build<ComplexVisualPlayer>::create(this, false)
        .parent(this)
        .id("visual-player1"_spr)
        .collect();

    this->player2 = Build<ComplexVisualPlayer>::create(this, true)
        .parent(this)
        .id("visual-player2"_spr)
        .collect();

    return true;
}

void RemotePlayer::updateAccountData(const PlayerAccountData& data, bool force) {
    if (!force && this->accountData == data) {
        defaultTicks = 0;
        return;
    }

    this->accountData = data;
    player1->updateIcons(data.icons);
    player2->updateIcons(data.icons);

    player1->updateName();
    player2->updateName();

    if (progressIcon) {
        progressIcon->updateIcons(data.icons);
    }

    if (progressArrow) {
        progressArrow->updateIcons(data.icons);
    }

    defaultTicks = 0;
}

const PlayerAccountData& RemotePlayer::getAccountData() const {
    return accountData;
}

void RemotePlayer::updateData(
        const VisualPlayerState& data,
        FrameFlags frameFlags,
        bool speaking,
        float loudness
) {
    player1->updateData(data.player1, data, speaking, loudness);
    player2->updateData(data.player2, data, speaking, loudness);

    isEditorBuilding = data.isEditorBuilding;

    lastPercentage = data.currentPercentage;

    // don't update any anims if hidden
    if (isForciblyHidden) return;

    if (frameFlags.pendingDeath && GlobedSettings::get().players.deathEffects) {
        player1->playDeathEffect();
    }

    if (frameFlags.pendingP1Teleport) {
        player1->playSpiderTeleport(frameFlags.pendingP1Teleport.value());
    }

    if (frameFlags.pendingP2Teleport) {
        player2->playSpiderTeleport(frameFlags.pendingP2Teleport.value());
    }

    if (frameFlags.pendingP1Jump) {
        player1->playJump();
    }

    if (frameFlags.pendingP2Jump) {
        player2->playJump();
    }
}

void RemotePlayer::updateProgressIcon() {
    if (progressIcon) {
        progressIcon->updatePosition(lastPercentage);

        if (isForciblyHidden || isEditorBuilding) {
            progressIcon->setVisible(false);
        }
    }
}

void RemotePlayer::updateProgressArrow(
        cocos2d::CCPoint cameraOrigin,
        cocos2d::CCSize cameraCoverage,
        cocos2d::CCPoint visibleOrigin,
        cocos2d::CCSize visibleCoverage,
        float zoom
) {
    if (progressArrow) {
        progressArrow->updatePosition(cameraOrigin, cameraCoverage, visibleOrigin, visibleCoverage, player1->getPlayerPosition(), zoom);

        if (isForciblyHidden || isEditorBuilding) {
            progressArrow->setVisible(false);
        }
    }
}

void RemotePlayer::onExit() {
    // do nothing.
}

unsigned int RemotePlayer::getDefaultTicks() {
    return defaultTicks;
}

void RemotePlayer::setDefaultTicks(unsigned int ticks) {
    defaultTicks = ticks;
}

void RemotePlayer::incDefaultTicks() {
    defaultTicks++;
}

bool RemotePlayer::isValidPlayer() {
    return accountData.accountId != 0;
}

void RemotePlayer::setForciblyHidden(bool state) {
    isForciblyHidden = state;
    player1->setForciblyHidden(state);
    player2->setForciblyHidden(state);

    if (progressArrow) {
        progressArrow->setVisible(!isForciblyHidden);
    }

    if (progressIcon) {
        progressIcon->setVisible(!isForciblyHidden);
    }
}

bool RemotePlayer::getForciblyHidden() {
    return isForciblyHidden;
}

void RemotePlayer::removeProgressIndicators() {
    if (progressIcon) {
        progressIcon->removeFromParent();
        progressIcon = nullptr;
    }

    if (progressArrow) {
        progressArrow->removeFromParent();
        progressArrow = nullptr;
    }
}

RemotePlayer* RemotePlayer::create(PlayerProgressIcon* progressIcon, PlayerProgressArrow* progressArrow, const PlayerAccountData& data) {
    auto ret = new RemotePlayer;
    if (ret->init(progressIcon, progressArrow, data)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

RemotePlayer* RemotePlayer::create(PlayerProgressIcon* progressIcon, PlayerProgressArrow* progressArrow) {
    return create(progressIcon, progressArrow, PlayerAccountData::DEFAULT_DATA);
}