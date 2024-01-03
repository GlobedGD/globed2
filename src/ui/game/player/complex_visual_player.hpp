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
};

class ComplexVisualPlayer : public cocos2d::CCNode, public BaseVisualPlayer {
public:
    bool init(RemotePlayer* parent, bool isSecond) override;
    void updateIcons(const PlayerIconData& icons) override;
    void updateData(const SpecificIconData& data) override;
    void updateIconType(PlayerIconType newType) override;

    static ComplexVisualPlayer* create(RemotePlayer* parent, bool isSecond);

protected:
    ComplexPlayerObject* playerIcon;
    PlayerIconType playerIconType;
};