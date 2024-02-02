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

void RemotePlayer::updateAccountData(const PlayerAccountData& data) {
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

void RemotePlayer::updateData(const VisualPlayerState& data, bool playDeathEffect) {
    player1->updateData(data.player1, data.isDead);
    player2->updateData(data.player2, data.isDead);

    if (playDeathEffect && GlobedSettings::get().players.deathEffects) {
        player1->playDeathEffect();
    }

    lastXPosition = data.player1.position.x;
}

void RemotePlayer::updateProgressIcon() {
    progressIcon->updatePosition(lastXPosition);
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