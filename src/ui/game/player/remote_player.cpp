#include "remote_player.hpp"

#include <UIBuilder.hpp>

using namespace geode::prelude;

bool RemotePlayer::init(const PlayerAccountData& data) {
    if (!CCNode::init()) return false;
    this->accountData = data;
    this->player1 = VisualPlayer::create(this, false);
    this->player2 = VisualPlayer::create(this, true);

    return true;
}

void RemotePlayer::updateAccountData(const PlayerAccountData& data) {
    this->accountData = data;
    player1->updateIcons(data.icons);
    player2->updateIcons(data.icons);
}

const PlayerAccountData& RemotePlayer::getAccountData() const {
    return accountData;
}

void RemotePlayer::updateData(const PlayerData& data) {
    player1->updateData(data.player1);
    player2->updateData(data.player2);
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