#pragma once
#include <Geode/Geode.hpp>
#include "signup_layer.hpp"

class GlobedMenuLayer : public cocos2d::CCLayer {
public:
    static constexpr float LIST_WIDTH = 358.f;
    static constexpr float LIST_HEIGHT = 220.f;

    bool init();
    cocos2d::CCArray* createServerList();
    void refreshServerList(float _);
    void requestServerList();
    void keyBackClicked();
    void pingServers(float _);

    static GlobedMenuLayer* create() {
        auto ret = new GlobedMenuLayer;
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }

        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    static cocos2d::CCScene* scene() {
        auto layer = GlobedMenuLayer::create();
        auto scene = cocos2d::CCScene::create();
        layer->setPosition({0.f, 0.f});
        scene->addChild(layer);

        return scene;
    }

private:
    GJListLayer* listLayer;
    GlobedSignupLayer* signupLayer;
};