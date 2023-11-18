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
private:
    static constexpr const char* CONSENT_MESSAGE =
        "For verification purposes, this action will cause your account to leave a <cy>comment</c> on a certain level. "
        "There is nothing else that has to be done from your side, and once the verification is complete, "
        "the comment will be <cg>automatically deleted</c>. "
        "If you <cr>do not consent</c> to this action, press 'Cancel'.";
};