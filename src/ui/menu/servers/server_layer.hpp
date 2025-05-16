#pragma once

#include <defs/geode.hpp>

#include "server_list.hpp"
#include <managers/web.hpp>

class GLOBED_DLL GlobedServersLayer : public cocos2d::CCLayer {
public:
    static GlobedServersLayer* create();

private:
    cocos2d::CCSprite* background;
    GlobedServerList* serverList;
    WebRequestManager::Listener requestListener;
    bool transitioningAway = false;
    bool serversLoaded = false;
    bool initializing = true;
    bool prevShownRelays = false;
    std::optional<int> serversLoadingFor;

    bool init() override;
    void onExit() override;
    void keyBackClicked() override;

    void updateServerList(float dt);
    void requestServerList();
    void pingServers(float dt);
    void transitionToMainLayer();

    void cancelWebRequest();
    void requestCallback(typename WebRequestManager::Event* event);
};
