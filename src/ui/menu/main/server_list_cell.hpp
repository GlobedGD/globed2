#pragma once
#include <Geode/Geode.hpp>
#include <managers/server_manager.hpp>

class ServerListCell : public cocos2d::CCLayer {
public:
    bool init(const GameServerView& gsview);

    static ServerListCell* create(const GameServerView& gsview) {
        auto ret = new ServerListCell;
        if (ret && ret->init(gsview)) {
            return ret;
        }

        CC_SAFE_DELETE(ret);
        return nullptr;
    }

private:
    GameServerView gsview;
};