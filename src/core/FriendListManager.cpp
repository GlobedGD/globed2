#include <globed/core/FriendListManager.hpp>
#include <globed/core/PopupManager.hpp>

using namespace geode::prelude;

namespace globed {

FriendListManager::FriendListManager() {}

void FriendListManager::refresh(bool force) {
    if ((m_fetched && !force) || m_loadStep != LoadStep::None) return;

    auto glm = GameLevelManager::get();
    glm->m_userListDelegate = this;
    m_loadStep = LoadStep::Friends;

    glm->getUserList(UserListType::Friends);
}

bool FriendListManager::isFriend(int userId) {
    return m_friends.contains(userId);
}

bool FriendListManager::isBlocked(int userId) {
    return m_blocked.contains(userId);
}

bool FriendListManager::isLoaded() {
    return m_fetched;
}

void FriendListManager::getUserListFinished(CCArray* p0, UserListType p1) {
    auto& set = p1 == UserListType::Friends ? m_friends : m_blocked;
    set.clear();

    for (auto user : CCArrayExt<GJUserScore>(p0)) {
        set.insert(user->m_accountID);
    }

    log::debug("Loaded {} {}", set.size(), p1 == UserListType::Friends ? "friends" : "blocked users");

    this->advance();
}

void FriendListManager::getUserListFailed(UserListType p0, GJErrorCode p1) {
    int error = (int)p1;
    if (error == -2) {
        // no friends or blocked users
    } else {
        globed::toastError("Failed to fetch {}: code {}", p0 == UserListType::Friends ? "friends" : "blocked users", error);
    }

    this->advance();
}

void FriendListManager::advance() {
    switch (m_loadStep) {
        case LoadStep::Friends: {
            m_loadStep = LoadStep::Blocked;
        } break;

        case LoadStep::Blocked: {
            m_loadStep = LoadStep::None;
            m_fetched = true;
        } break;

        default: break;
    }
}

}
