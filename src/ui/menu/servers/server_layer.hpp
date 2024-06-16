#pragma once

#include <defs/geode.hpp>

#include "signup_layer.hpp"
#include "server_list.hpp"
#include <managers/web.hpp>

class GlobedServersLayer : public cocos2d::CCLayer {
public:

    static GlobedServersLayer* create();

private:
    cocos2d::CCSprite* background;
    GlobedServerList* serverList;
    GlobedSignupLayer* signupLayer;
    WebRequestManager::Listener requestListener;
    bool transitioningAway = false;

    bool init() override;
    void onExit() override;
    void keyBackClicked() override;

    void updateBG(float dt);

    void updateServerList(float dt);
    void requestServerList();
    void pingServers(float dt);
    void transitionToMainLayer();

    void cancelWebRequest();
    void requestCallback(typename WebRequestManager::Event* event);
};
