#pragma once
#include <defs.hpp>

class PlayerStatusIcons : public cocos2d::CCNode {
public:
    void updateStatus(bool paused, bool practicing);

    static PlayerStatusIcons* create();

private:
    cocos2d::CCNode* iconWrapper = nullptr;
    bool wasPaused = false, wasPracticing = false;
    float nameScale = 0.f;
    bool init() override;
};