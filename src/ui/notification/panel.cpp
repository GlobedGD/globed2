#include "panel.hpp"

#include "invite.hpp"
#include <net/manager.hpp>
#include <hooks/gjbasegamelayer.hpp>
#include <util/singleton.hpp>

using namespace geode::prelude;
using namespace asp::time;

bool GlobedNotificationPanel::init() {
    if (!CCNode::init()) return false;

    this->setLayout(ColumnLayout::create()->setGap(-10.f)->setAutoScale(false)->setAxisAlignment(AxisAlignment::End));
    this->setContentSize({200.f, CCDirector::get()->getWinSize().height});
    this->setZOrder(99);

    this->scheduleUpdate();

    return true;
}

GlobedNotificationPanel* GlobedNotificationPanel::get() {
    return instance;
}

void GlobedNotificationPanel::persist() {
    this->setAnchorPoint({1.f, 1.f});
    this->setPosition(CCDirector::get()->getWinSize());

    SceneManager::get()->keepAcrossScenes(this);
    instance = this;
}

void GlobedNotificationPanel::addInviteNotification(uint32_t roomID, std::string_view password, const PlayerPreviewAccountData& player) {
    auto* notif = GlobedInviteNotification::create(roomID, password, player);
    this->slideInNotification(notif);

    auto* engine = FMODAudioEngine::sharedEngine();
    engine->playEffect("invite-sound.ogg"_spr, 1.f, 1.f, 1.f);
}

static bool shouldShowNotification() {
    auto* scene = CCScene::get();
    if (!scene || scene->getChildrenCount() == 0) return false;

    if (typeinfo_cast<CCTransitionScene*>(scene) || (scene->getChildByType<PlayLayer>(0) && !scene->getChildByType<PauseLayer>(0))) {
        return false;
    }

    return true;
}

void GlobedNotificationPanel::slideInNotification(CCNode* node) {
    auto time = SystemTime::now();
    if ((time - lastNotificationAdded).value() < NOTIFICATION_BUFFER_TIME || !shouldShowNotification()) {
        this->queueNotification(node);
        return;
    }

    lastNotificationAdded = time;
    // remove oldest notification if >3

    if (this->getChildrenCount() >= 3) {
        Ref<CCNode> notif = static_cast<CCNode*>(this->getChildren()->objectAtIndex(0));
        notif->removeFromParent();
        this->getParent()->addChild(notif);
        notif->setPosition(this->convertToWorldSpace(notif->getPosition()));

        notif->runAction(
            CCSequence::create(
                CCEaseSineIn::create(CCMoveBy::create(0.6f, {notif->getScaledContentSize().width, 0.f})),
                CCCallFunc::create(notif, callfunc_selector(CCNode::removeFromParent)),
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

void GlobedNotificationPanel::queueNotification(cocos2d::CCNode* node) {
    queuedNotifs.push(node);
}

void GlobedNotificationPanel::update(float dt) {
    // hide if in a level
    bool shouldShow = true;
    if (auto* pl = globed::cachedSingleton<GameManager>()->m_playLayer) {
        shouldShow = static_cast<GlobedGJBGL*>(static_cast<GJBaseGameLayer*>(pl))->isPaused();
    }

    this->setVisible(shouldShow);

    // remove all invites if we disconnected
    auto& nm = NetworkManager::get();
    if (!nm.established()) {
        this->removeAllChildren();
        queuedNotifs = {};
        return;
    }

    if (queuedNotifs.empty()) return;

    if (lastNotificationAdded.elapsed() < NOTIFICATION_BUFFER_TIME) return;

    this->slideInNotification(queuedNotifs.front());
    queuedNotifs.pop();
}

GlobedNotificationPanel* GlobedNotificationPanel::create() {
    auto ret = new GlobedNotificationPanel;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
