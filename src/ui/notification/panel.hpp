#pragma once

#include <queue>

#include <defs/geode.hpp>
#include <data/types/gd.hpp>
#include <util/time.hpp>

class GlobedNotificationPanel : public cocos2d::CCNode {
public:
    static GlobedNotificationPanel* create();
    static GlobedNotificationPanel* get();
    inline static GlobedNotificationPanel* instance = nullptr;

    void persist();

    void addInviteNotification(uint32_t roomID, uint32_t roomToken, const PlayerRoomPreviewAccountData& player);
    void slideInNotification(cocos2d::CCNode* node);
    void queueNotification(cocos2d::CCNode* node);

private:
    static constexpr auto NOTIFICATION_BUFFER_TIME = util::time::seconds(2);

    util::time::time_point lastNotificationAdded;
    std::queue<Ref<cocos2d::CCNode>> queuedNotifs;

    bool init() override;
    void update(float dt) override;
};
