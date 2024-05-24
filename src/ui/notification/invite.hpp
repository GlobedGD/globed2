#pragma once

#include <defs/geode.hpp>
#include <data/types/gd.hpp>

#include <ui/general/simple_player.hpp>

class GlobedInviteNotification : public cocos2d::CCLayer {
public:
    static GlobedInviteNotification* create(uint32_t roomID, const std::string_view password, const PlayerPreviewAccountData& player);

private:
    bool init(uint32_t roomID, const std::string_view password, const PlayerPreviewAccountData& player);
    void removeFromParent() override;
};
