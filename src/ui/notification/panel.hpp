#pragma once

#include <queue>

#include <defs/geode.hpp>
#include <data/types/gd.hpp>

#include <asp/time/SystemTime.hpp>

class GlobedNotificationPanel : public cocos2d::CCNode {
public:
    static GlobedNotificationPanel* create();
    static GlobedNotificationPanel* get();
    inline static GlobedNotificationPanel* instance = nullptr;

    void persist();

    void addInviteNotification(uint32_t roomID, std::string_view password, const PlayerPreviewAccountData& player);
    void slideInNotification(cocos2d::CCNode* node);
    void queueNotification(cocos2d::CCNode* node);

private:
    static constexpr auto NOTIFICATION_BUFFER_TIME = asp::time::Duration::fromMillis(1250);

    asp::time::SystemTime lastNotificationAdded = asp::time::SystemTime::UNIX_EPOCH;
    std::queue<Ref<cocos2d::CCNode>> queuedNotifs;

    bool init() override;
    void update(float dt) override;
};
