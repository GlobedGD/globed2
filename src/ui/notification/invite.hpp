#pragma once

#include <defs/geode.hpp>
#include <data/types/gd.hpp>

#include <ui/general/simple_player.hpp>

class GlobedInviteNotification : public cocos2d::CCLayer {
public:
    static GlobedInviteNotification* create(uint32_t roomID, uint32_t roomToken, const PlayerRoomPreviewAccountData& player);

private:
    bool init(uint32_t roomID, uint32_t roomToken, const PlayerRoomPreviewAccountData& player);
    void removeFromParent() override;
};
