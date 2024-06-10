#pragma once
#include <defs/all.hpp>

#include <Geode/utils/web.hpp>

#include "signup_layer.hpp"
#include <managers/web.hpp>

class GlobedMenuLayer : public cocos2d::CCLayer {
public:
    static constexpr float LIST_WIDTH = 358.f;
    static constexpr float LIST_HEIGHT = 220.f;

    void onExit() override;

    static GlobedMenuLayer* create();

private:
    GJListLayer* listLayer;
    GlobedSignupLayer* signupLayer;
    Ref<CCMenuItemSpriteExtra> levelListButton, roomButton, serverSwitcherButton, discordButton, settingsButton;
    cocos2d::CCMenu *leftButtonMenu, *rightButtonMenu;
    WebRequestManager::Listener requestListener;

    bool currentlyShowingButtons = false;

    bool init() override;
    cocos2d::CCArray* createServerList();
    void refreshServerList(float dt);
    void requestServerList();
    void keyBackClicked() override;
    void keyDown(cocos2d::enumKeyCodes key) override;
    void pingServers(float dt);

    void cancelWebRequest();
    void requestCallback(typename WebRequestManager::Event* event);
};
