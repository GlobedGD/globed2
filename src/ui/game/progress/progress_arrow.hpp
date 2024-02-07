#pragma once
#include <defs.hpp>

class PlayerProgressArrow : public cocos2d::CCNode {
public:
    static PlayerProgressArrow* create();

    // cameraOrigin is the bottom-left corner of the screen basically
    void updatePosition(cocos2d::CCPoint cameraOrigin, cocos2d::CCSize cameraCoverage, cocos2d::CCPoint playerPosition);

private:

    bool init() override;
};
