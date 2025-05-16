#pragma once

#include <defs/geode.hpp>

#include "server_list_cell.hpp"
#include <ui/general/list/list.hpp>

class GlobedServerList : public cocos2d::CCLayer {
public:
    static constexpr float LIST_WIDTH = 358.f;
    static constexpr float LIST_HEIGHT = 220.f;

    bool showingRelays = false;

    void forceRefresh();
    void softRefresh();

    static GlobedServerList* create();

private:
    using ServerList = GlobedListLayer<cocos2d::CCNode>;

    GJListLayer* bgListLayer;
    ServerList* listLayer;

    bool init() override;

    cocos2d::CCArray* createServerList();
};
