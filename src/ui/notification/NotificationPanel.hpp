#pragma once

#include <globed/core/data/Messages.hpp>
#include <globed/core/net/MessageListener.hpp>

#include <Geode/Geode.hpp>
#include <queue>
#include <asp/time/SystemTime.hpp>

namespace globed {

class NotificationPanel : public cocos2d::CCNode {
public:
    static NotificationPanel* get();

    void queueInviteNotification(const msg::InvitedMessage& msg);
    void queueNotification(CCNode* notif);
    void onNotificationRemoved();

private:
    static constexpr auto NOTIFICATION_BUFFER_TIME = asp::time::Duration::fromMillis(1250);

    asp::time::SystemTime m_lastAdded = asp::time::SystemTime::UNIX_EPOCH;
    std::queue<geode::Ref<CCNode>> m_queued;
    std::optional<MessageListener<msg::InvitedMessage>> m_listener;

    bool init() override;
    void update(float dt) override;
    void slideInNotification(CCNode* notif);
};

}