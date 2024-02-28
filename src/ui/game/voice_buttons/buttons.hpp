#pragma once
#include <defs/all.hpp>

class VoiceChatButtonsMenu : public cocos2d::CCNode {
public:
    static VoiceChatButtonsMenu* create();

private:
    cocos2d::CCMenu* btnMenu;
    cocos2d::extension::CCScale9Sprite* background;

    bool init() override;
};