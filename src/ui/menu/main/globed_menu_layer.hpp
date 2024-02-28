#pragma once
#include <defs/all.hpp>

#include "signup_layer.hpp"
#include <Geode/utils/web.hpp>

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
    cocos2d::CCMenu* leftButtonMenu;
    std::optional<geode::utils::web::SentAsyncWebRequestHandle> serverRequestHandle;
    cocos2d::CCSequence* timeoutSequence;
    bool currentlyShowingButtons = false;

    bool init() override;
    cocos2d::CCArray* createServerList();
    void refreshServerList(float dt);
    void requestServerList();
    void keyBackClicked() override;
    void keyDown(cocos2d::enumKeyCodes key) override;
    void pingServers(float dt);

    void cancelWebRequest();
};