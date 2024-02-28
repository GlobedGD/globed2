#pragma once
#include <defs/all.hpp>

#include <data/types/gd.hpp>

class PlayerProgressArrow : public cocos2d::CCNode {
public:
    static PlayerProgressArrow* create();

    // cameraOrigin is the bottom-left corner of the screen in objectLayer
    // visibleOrigin and visibleCoverage is just 0,0 and getWinSize()
    void updatePosition(
        cocos2d::CCPoint cameraOrigin,
        cocos2d::CCSize cameraCoverage,
        cocos2d::CCPoint visibleOrigin,
        cocos2d::CCSize visibleCoverage,
        cocos2d::CCPoint playerPosition,
        float zoom
    );

    void updateIcons(const PlayerIconData& data);

private:
    SimplePlayer* playerIcon = nullptr;

    bool init() override;
};
