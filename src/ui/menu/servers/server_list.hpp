#pragma once

#include <defs/geode.hpp>

class GlobedServerList : public cocos2d::CCLayer {
public:
    static constexpr float LIST_WIDTH = 358.f;
    static constexpr float LIST_HEIGHT = 220.f;

    void forceRefresh();
    void softRefresh();

    static GlobedServerList* create();

private:
    GJListLayer* listLayer;

    bool init() override;

    cocos2d::CCArray* createServerList();
};
