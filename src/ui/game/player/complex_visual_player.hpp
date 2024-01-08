#pragma once
#include <Geode/modify/PlayerObject.hpp>

#include "base_visual_player.hpp"

class $modify(ComplexPlayerObject, PlayerObject) {
    // those are needed so that our changes don't impact actual PlayerObject instances
    bool isGlobedPlayer = false;

    void setRemoteState() {
        m_fields->isGlobedPlayer = true;
    }

    bool vanilla() {
        return !m_fields->isGlobedPlayer;
    }

    void incrementJumps() {
        if (vanilla()) PlayerObject::incrementJumps();
    }
};

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