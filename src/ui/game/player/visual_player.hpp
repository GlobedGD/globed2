#pragma once
#include <defs.hpp>

#include <data/types/gd.hpp>

class RemotePlayer;

class VisualPlayer : public cocos2d::CCNode {
public:
    bool init(RemotePlayer* parent, bool isSecond);
    void updateIcons(const PlayerIconData& icons);
    void updateData(const SpecificIconData& data);
    void updateIconType(PlayerIconType newType);

    static VisualPlayer* create(RemotePlayer* parent, bool isSecond);
protected:
    RemotePlayer* parent;
    SimplePlayer* playerIcon;
    bool isSecond;
    PlayerIconType playerIconType;
};