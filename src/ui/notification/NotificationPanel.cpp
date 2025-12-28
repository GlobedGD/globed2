#include "NotificationPanel.hpp"
#include "InviteNotification.hpp"
#include <globed/util/singleton.hpp>
#include <globed/core/FriendListManager.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>

using namespace geode::prelude;
using namespace asp::time;

namespace globed {

static bool allowInvite(int sender) {
    auto setting = (InvitesFrom)globed::setting<int>("core.invites-from").value();
    auto& flm = FriendListManager::get();

    // block the invite if either..
    return !(
        // a. invites are disabled
        setting == InvitesFrom::Nobody
        // b. invites are friend-only and the user is not a friend
        || (setting == InvitesFrom::Friends && !flm.isFriend(sender))
        // c. user is blocked
        || flm.isBlocked(sender)
    );
}

bool NotificationPanel::init() {
    CCNode::init();

    auto winSize = CCDirector::get()->getWinSize();

    this->setID("notification-panel"_spr);
    this->setAnchorPoint({1.f, 1.f});
    this->setPosition(winSize);
    this->setLayout(ColumnLayout::create()->setGap(-10.f)->setAutoScale(false)->setAxisAlignment(AxisAlignment::End));
    this->setContentSize({200.f, winSize.height});
    this->setZOrder(99);

    this->scheduleUpdate();

    SceneManager::get()->keepAcrossScenes(this);

    m_listener = NetworkManagerImpl::get().listen<msg::InvitedMessage>([this](const auto& msg) {
        if (!allowInvite(msg.invitedBy.accountId)) {
            return ListenerResult::Stop; // halt early
        }

        this->queueInviteNotification(msg);
        return ListenerResult::Continue;
    });
    m_listener->setPriority(-100);

    return true;
}

static bool shouldShowNotification() {
    auto* scene = CCScene::get();
    if (!scene || scene->getChildrenCount() == 0) return false;

    if (typeinfo_cast<CCTransitionScene*>(scene) || (scene->getChildByType<PlayLayer>(0) && !scene->getChildByType<PauseLayer>(0))) {
        return false;
    }

    return true;
}

void NotificationPanel::update(float dt) {
    // hide if in a level or if no notifications are to be shown
    bool shouldShow = this->getChildrenCount() > 0 || !m_queued.empty();

    if (shouldShow) {
        if (auto* pl = globed::cachedSingleton<GameManager>()->m_playLayer) {
            shouldShow = static_cast<GlobedGJBGL*>(static_cast<GJBaseGameLayer*>(pl))->isPaused();
        }
    }

    this->setVisible(shouldShow);

    // remove all invites if we disconnected
    auto& nm = NetworkManagerImpl::get();
    if (!nm.isConnected()) {
        this->removeAllChildren();
        m_queued = {};
        return;
    }

    if (m_queued.empty()) return;

    auto time = SystemTime::now();
    if (m_lastAdded.elapsed() < NOTIFICATION_BUFFER_TIME || !shouldShowNotification()) {
        return;
    }

    auto notif = m_queued.front();
    m_queued.pop();
    this->slideInNotification(notif);
}

void NotificationPanel::slideInNotification(CCNode* node) {
    m_lastAdded = SystemTime::now();

    if (this->getChildrenCount() >= 3) {
        Ref<CCNode> notif = this->getChildrenExt()[0];
        notif->removeFromParent();
        this->getParent()->addChild(notif);
        notif->setPosition(this->convertToWorldSpace(notif->getPosition()));

        notif->runAction(
            CCSequence::create(
                CCEaseSineIn::create(CCMoveBy::create(0.6f, {notif->getScaledContentSize().width, 0.f})),
                // CCCallFunc::create(notif, callfunc_selector(CCNode::removeFromParent)),
                CCRemoveSelf::create(),
                nullptr
            )
        );
    }

    std::vector<CCPoint> oldPositions;

    for (auto* child : CCArrayExt<CCNode*>(this->getChildren())) {
        oldPositions.push_back(child->getPosition());
    }

    this->addChild(node);
    this->updateLayout();

    size_t childIdx = 0;
    for (auto* child : CCArrayExt<CCNode*>(this->getChildren())) {
        if (childIdx >= oldPositions.size()) break;

        auto dest = child->getPosition();
        const auto& src = oldPositions.at(childIdx++);
        child->setPosition(src);

        child->runAction(
            CCEaseSineInOut::create(CCMoveTo::create(0.6f, dest))
        );
    }

    auto dest = node->getPosition();
    auto src = CCPoint{dest.x + node->getScaledContentSize().width, dest.y};

    node->setPosition(src);
    auto* action = CCEaseSineOut::create(CCMoveTo::create(0.6f, dest));
    node->runAction(action);
}

void NotificationPanel::queueInviteNotification(const msg::InvitedMessage& msg) {
    auto node = InviteNotification::create(msg);
    this->queueNotification(node);
}

void NotificationPanel::queueNotification(CCNode* notif) {
    m_queued.push(notif);
}

void NotificationPanel::onNotificationRemoved() {
    this->updateLayout();
}

NotificationPanel* NotificationPanel::get() {
    static NotificationPanel* instance = nullptr;

    if (!instance) {
        auto ret = new NotificationPanel;
        ret->init();
        ret->autorelease();
        instance = ret;
    }

    return instance;
}

$on_mod(Loaded) {
    NotificationPanel::get();
}

}