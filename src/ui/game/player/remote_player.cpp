#include "remote_player.hpp"

#include <managers/settings.hpp>

using namespace geode::prelude;

bool RemotePlayer::init(PlayerProgressIcon* progressIcon, const PlayerAccountData& data) {
    if (!CCNode::init()) return false;
    this->accountData = data;
    this->progressIcon = progressIcon;

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

    defaultTicks = 0;
}

const PlayerAccountData& RemotePlayer::getAccountData() const {
    return accountData;
}

void RemotePlayer::updateData(const VisualPlayerState& data, bool playDeathEffect, bool speaking, std::optional<SpiderTeleportData> p1tp, std::optional<SpiderTeleportData> p2tp) {
    player1->updateData(data.player1, data.isDead, data.isPaused, data.isPracticing, speaking);
    player2->updateData(data.player2, data.isDead, data.isPaused, data.isPracticing, speaking);

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
    progressIcon->updatePosition(lastPercentage);
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
    return accountData.id != 0;
}

RemotePlayer* RemotePlayer::create(PlayerProgressIcon* progressIcon, const PlayerAccountData& data) {
    auto ret = new RemotePlayer;
    if (ret->init(progressIcon, data)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

RemotePlayer* RemotePlayer::create(PlayerProgressIcon* progressIcon) {
    return create(progressIcon, PlayerAccountData::DEFAULT_DATA);
}