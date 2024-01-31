#pragma once
#include <defs.hpp>

class PlayerProgressIcon : public cocos2d::CCNode {
public:
    static PlayerProgressIcon* create(float maxLevelX);

    constexpr static float calculateRatio(float xPosition, float maxX) {
        return std::clamp(xPosition / (maxX + 50.f), 0.f, 0.99f);
    }

    void updatePosition(float xPosition);
    void toggleLine(bool enabled);

private:
    float maxX;

    bool init(float maxLevelX);
};
