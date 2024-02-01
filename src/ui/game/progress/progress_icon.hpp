#pragma once
#include <defs.hpp>

#include <data/types/gd.hpp>

class PlayerProgressIcon : public cocos2d::CCNode {
public:
    static PlayerProgressIcon* create(float maxLevelX);

    constexpr static float calculateRatio(float xPosition, float maxX) {
        return std::clamp(xPosition / (maxX + 50.f), 0.f, 0.99f);
    }

    void updateMaxLevelX(float maxLevelX);
    void updateIcons(const PlayerIconData& data);
    void updatePosition(float xPosition);
    void toggleLine(bool enabled);

private:
    cocos2d::CCLayerColor* line = nullptr;
    SimplePlayer* playerIcon = nullptr;
    float maxX;

    bool init(float maxLevelX);
};
