#pragma once

#include "base_visual_player.hpp"
#include <hooks/player_object.hpp>

class ComplexVisualPlayer : public cocos2d::CCNode, public BaseVisualPlayer {
public:
    bool init(RemotePlayer* parent, bool isSecond) override;
    void updateIcons(const PlayerIconData& icons) override;
    void updateData(const SpecificIconData& data) override;
    void updateName() override;
    void updateIconType(PlayerIconType newType) override;
    void playDeathEffect() override;

    void updatePlayerObjectIcons();
    void toggleAllOff();
    void callToggleWith(PlayerIconType type, bool arg1, bool arg2);
    void callUpdateWith(PlayerIconType type, int icon);

    static ComplexVisualPlayer* create(RemotePlayer* parent, bool isSecond);

protected:
    PlayLayer* playLayer;
    ComplexPlayerObject* playerIcon;
    PlayerIconType playerIconType = PlayerIconType::Unknown;
    cocos2d::CCLabelBMFont* playerName;

    PlayerIconData storedIcons;
};