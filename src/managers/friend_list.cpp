#include "friend_list.hpp"

#include <managers/error_queues.hpp>

void FriendListManager::load() {
    auto* glm = GameLevelManager::sharedState();
    if (!dummyNode) {
        dummyNode = DummyNode::create();
    }

    glm->m_userListDelegate = dummyNode;
    glm->getUserList(UserListType::Friends);
}

bool FriendListManager::isLoaded() {
    return loaded;
}

void FriendListManager::maybeLoad() {
    if (!this->isLoaded()) {
        this->load();
    }
}

void FriendListManager::invalidate() {
    friends.clear();
    loaded = false;
}

bool FriendListManager::isFriend(int playerId) {
    return friends.contains(playerId);
}

void FriendListManager::insertPlayers(cocos2d::CCArray* players) {
    for (auto* elem : CCArrayExt<GJUserScore*>(players)) {
        friends.insert(elem->m_accountID);
    }

    loaded = true;
}

void FriendListManager::DummyNode::cleanup() {
    GameLevelManager::sharedState()->m_userListDelegate = nullptr;
}

void FriendListManager::DummyNode::getUserListFinished(cocos2d::CCArray* p0, UserListType p1) {
    if (p1 == UserListType::Friends) {
        auto& flm = FriendListManager::get();
        flm.insertPlayers(p0);
    }
    this->cleanup();
}

void FriendListManager::DummyNode::getUserListFailed(UserListType p0, GJErrorCode p1) {
    // -2 means no friends :broken_heart:
    if ((int)p1 != -2) {
        ErrorQueues::get().warn(fmt::format("Failed to get friendlist: {}", (int)p1));
    }
    this->cleanup();
}

void FriendListManager::DummyNode::userListChanged(cocos2d::CCArray* p0, UserListType p1) {
    log::warn("why did changed get called");
    this->getUserListFinished(p0, p1);
}

void FriendListManager::DummyNode::forceReloadList(UserListType p0) {}

// ugly ass
FriendListManager::DummyNode* FriendListManager::DummyNode::create() {
    auto ret = new DummyNode;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
