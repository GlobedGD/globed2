#pragma once
#include <defs/all.hpp>

#include <data/types/gd.hpp>

class PlayerProgressIcon : public cocos2d::CCNode {
public:
    static PlayerProgressIcon* create();

    void updateIcons(const PlayerIconData& data);
    void updatePosition(float xPosition);
    void toggleLine(bool enabled);
    void setForceOnTop(bool state);

private:
    cocos2d::CCLayerColor* line = nullptr;
    SimplePlayer* playerIcon = nullptr;
    bool forceOnTop = false;

    bool init();
};
