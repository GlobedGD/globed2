#pragma once
#include <defs/all.hpp>

#include <data/types/gd.hpp>

using namespace geode::prelude;

class ChatOverlayCell : public cocos2d::CCNode {
public:
    static ChatOverlayCell* create(int accID, const std::string& message);
    void updateChat(float dt);
    void forceRemove();

    float delta = 7.f;
    CCScale9Sprite* cc9s;

private:
    cocos2d::CCLabelBMFont* messageText;
    cocos2d::CCMenu* nodeWrapper;

    bool init(int accID, const std::string& message);
};