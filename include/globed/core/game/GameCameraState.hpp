#pragma once
#include <cocos2d.h>
#include <cmath>

namespace globed {

struct CameraDirection {
    cocos2d::CCPoint vector;
    float angle;
};

struct GameCameraState {
    cocos2d::CCPoint cameraOrigin;
    cocos2d::CCPoint visibleOrigin;
    cocos2d::CCSize visibleCoverage;
    float zoom;

    inline cocos2d::CCSize cameraCoverage() const {
        return visibleCoverage / std::abs(zoom);
    }

    inline cocos2d::CCPoint cameraCenter() const {
        return cameraOrigin + this->cameraCoverage() / 2.f;
    }
};

}
