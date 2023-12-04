#pragma once
#include <Geode/Geode.hpp>
#include "signup_layer.hpp"

class GlobedMenuLayer : public cocos2d::CCLayer {
public:
    static constexpr float LIST_WIDTH = 358.f;
    static constexpr float LIST_HEIGHT = 220.f;

    static GlobedMenuLayer* create();
    static cocos2d::CCScene* scene();

private:
    GJListLayer *listLayer, *standaloneLayer;
    GlobedSignupLayer* signupLayer;

    bool init();
    cocos2d::CCArray* createServerList();
    cocos2d::CCArray* createStandaloneList();
    void refreshServerList(float _);
    void requestServerList();
    void keyBackClicked();
    void pingServers(float _);
};