#pragma once

#include <defs/geode.hpp>

#include "signup_layer.hpp"
#include "server_list.hpp"
#include <managers/web.hpp>

class GlobedServersLayer : public cocos2d::CCLayer {
public:

    static GlobedServersLayer* create();

private:
    GlobedServerList* serverList;
    GlobedSignupLayer* signupLayer;
    WebRequestManager::Listener requestListener;

    bool init() override;
    void onExit() override;
    void keyBackClicked() override;

    void updateServerList(float dt);
    void requestServerList();
    void pingServers(float dt);

    void cancelWebRequest();
    void requestCallback(typename WebRequestManager::Event* event);
};
