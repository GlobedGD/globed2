#include "remote_player.hpp"

#include <UIBuilder.hpp>

using namespace geode::prelude;

bool RemotePlayer::init(const PlayerAccountData& data) {
    if (!CCNode::init()) return false;
    this->accountData = data;

    this->player1 = Build<VisualPlayer>::create(this, false)
        .parent(this)
        .id("visual-player1"_spr)
        .collect();

    this->player2 = Build<VisualPlayer>::create(this, true)
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

    defaultTicks = 0;
}

const PlayerAccountData& RemotePlayer::getAccountData() const {
    return accountData;
}

void RemotePlayer::updateData(const VisualPlayerState& data) {
    player1->updateData(data.player1);
    player2->updateData(data.player2);
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

RemotePlayer* RemotePlayer::create(const PlayerAccountData& data) {
    auto ret = new RemotePlayer;
    if (ret->init(data)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

RemotePlayer* RemotePlayer::create() {
    return create(PlayerAccountData::DEFAULT_DATA);
}