#pragma once
#include <defs/all.hpp>

class GlobedSignupLayer : public cocos2d::CCLayer {
public:
    static constexpr float LIST_WIDTH = 358.f;
    static constexpr float LIST_HEIGHT = 220.f;

    static GlobedSignupLayer* create();

private:
    bool init();

    static constexpr const char* CONSENT_MESSAGE =
        "For verification purposes, this action will send a <cy>direct message</c> to a bot account, and delete it afterwards. "
        "If you <cr>do not consent</c> to this action, press 'Cancel'.";
};