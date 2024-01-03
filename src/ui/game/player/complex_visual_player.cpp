#include "complex_visual_player.hpp"

#include <UIBuilder.hpp>

#include "remote_player.hpp"
#include <util/misc.hpp>

using namespace geode::prelude;

bool ComplexVisualPlayer::init(RemotePlayer* parent, bool isSecond) {
    if (!CCNode::init() || !BaseVisualPlayer::init(parent, isSecond)) return false;

    auto& data = parent->getAccountData();

    playerIcon = static_cast<ComplexPlayerObject*>(Build<PlayerObject>::create(0, 0, nullptr, nullptr, true)
        .parent(this)
        .collect());

    playerIcon->setRemoteState();

    this->updateIcons(data.icons);

    return true;
}

void ComplexVisualPlayer::updateIcons(const PlayerIconData& icons) {
    auto* gm = GameManager::get();

    this->updateIconType(playerIconType);
}

void ComplexVisualPlayer::updateData(const SpecificIconData& data) {

}

void ComplexVisualPlayer::updateIconType(PlayerIconType newType) {
    playerIconType = newType;

    const auto& accountData = parent->getAccountData();
    const auto& icons = accountData.icons;

    // switch (playerIconType) {
    // case PlayerIconType::Cube:
    // playerIcon.toggle
    //     playerIcon->updatePlayerFrame(icons.cube)
    // }
}

ComplexVisualPlayer* ComplexVisualPlayer::create(RemotePlayer* parent, bool isSecond) {
    // auto ret = new ComplexVisualPlayer;
    // if (ret->init(parent, isSecond)) {
    //     ret->autorelease();
    //     return ret;
    // }

    // delete ret;
    return nullptr;
}