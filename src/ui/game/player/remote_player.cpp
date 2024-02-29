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
        bool playDeathEffect,
        bool speaking,
        std::optional<SpiderTeleportData> p1tp,
        std::optional<SpiderTeleportData> p2tp,
        float loudness
) {
    player1->updateData(data.player1, data, speaking, loudness);
    player2->updateData(data.player2, data, speaking, loudness);

    if (playDeathEffect && GlobedSettings::get().players.deathEffects) {
        player1->playDeathEffect();
    }

    if (p1tp) {
        player1->playSpiderTeleport(p1tp.value());
    }

    if (p2tp) {
        player2->playSpiderTeleport(p2tp.value());
    }

    lastPercentage = data.currentPercentage;
}

void RemotePlayer::updateProgressIcon() {
    if (progressIcon) {
        progressIcon->updatePosition(lastPercentage);
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
    }
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