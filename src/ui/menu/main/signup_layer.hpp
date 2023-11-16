#pragma once
#include <Geode/Geode.hpp>

class GlobedSignupLayer : public cocos2d::CCLayer {
public:
    static constexpr float LIST_WIDTH = 358.f;
    static constexpr float LIST_HEIGHT = 220.f;
    
    bool init();

    static GlobedSignupLayer* create() {
        auto ret = new GlobedSignupLayer;
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }

        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};