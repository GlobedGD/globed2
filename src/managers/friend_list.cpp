#include "friend_list.hpp"

#include <managers/error_queues.hpp>

void FriendListManager::load(bool friends) {
    auto* glm = GameLevelManager::sharedState();

    fetchState = friends ? FetchState::Friends : FetchState::Blocked;
    glm->m_userListDelegate = &dummyNode;
    glm->getUserList(friends ? UserListType::Friends : UserListType::Blocked);
}

bool FriendListManager::isFetching() {
    return fetchState != FetchState::None;
}

bool FriendListManager::areFriendsLoaded() {
    return loadedFriends;
}

bool FriendListManager::areBlockedLoaded() {
    return loadedBlocked;
}

void FriendListManager::maybeLoad() {
    if (!this->areFriendsLoaded()) {
        this->load(true);
    } else if (!this->areBlockedLoaded()) {
        this->load(false);
    }
}

void FriendListManager::invalidate() {
    listFriends.clear();
    listBlocked.clear();
    loadedFriends = false;
    loadedBlocked = false;
}

bool FriendListManager::isFriend(int playerId) {
    return listFriends.contains(playerId);
}

bool FriendListManager::isBlocked(int playerId) {
    return listBlocked.contains(playerId);
}

std::vector<int> FriendListManager::getFriendList() {
    std::vector<int> list;
    list.reserve(listFriends.size());

    for (auto& elem : listFriends) {
        list.push_back(elem);
    }

    return list;
}

void FriendListManager::insertPlayers(cocos2d::CCArray* players, bool friends) {
    auto& set = friends ? listFriends : listBlocked;

    for (auto* elem : CCArrayExt<GJUserScore*>(players)) {
        set.insert(elem->m_accountID);
    }

    // okay man
    friends ? (loadedFriends = true) : (loadedBlocked = true);
}

void FriendListManager::DummyNode::cleanup() {
    GameLevelManager::sharedState()->m_userListDelegate = nullptr;
    FriendListManager::get().fetchState = FetchState::None;
}

void FriendListManager::DummyNode::getUserListFinished(cocos2d::CCArray* p0, UserListType p1) {
    auto& flm = FriendListManager::get();
    flm.insertPlayers(p0, p1 == UserListType::Friends);

    this->cleanup();

    if (p1 == UserListType::Friends) {
        // fetch blocked users as well
        FriendListManager::get().maybeLoad();
    }
}

void FriendListManager::DummyNode::getUserListFailed(UserListType p0, GJErrorCode p1) {
    // -2 means no friends :broken_heart:
    if ((int)p1 == -2) {
        auto& flm = FriendListManager::get();
        if (flm.fetchState == FetchState::Friends) {
            flm.loadedFriends = true;
            flm.maybeLoad(); // load blocked users
            return;
        } else {
            flm.loadedBlocked = true;
        }
    } else {
        ErrorQueues::get().warn(fmt::format("[Globed] Failed to get friendlist: {}", (int)p1));
    }

    this->cleanup();
}
