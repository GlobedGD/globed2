#pragma once
#include <Geode/Geode.hpp>

class GlobedMenuLayer : public cocos2d::CCLayer {
public:
    static constexpr float LIST_CELL_HEIGHT = 45.0f;
    static constexpr float LIST_WIDTH = 358.f;
    static constexpr float LIST_HEIGHT = 190.f;

    bool init();
    cocos2d::CCArray* createServerList();
    void refreshServerList(float _);
    void requestServerList();
    void keyBackClicked();

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
};