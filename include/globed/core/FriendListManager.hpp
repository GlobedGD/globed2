#pragma once

#include <globed/util/singleton.hpp>
#include <asp/sync/Mutex.hpp>
#include <asp/sync/Atomic.hpp>
#include <Geode/Bindings.hpp>

namespace globed {

class GLOBED_DLL FriendListManager : public SingletonNodeBase<FriendListManager>, public UserListDelegate {
public:
    // Refresh cache if it was not yet fetched. Pass true to force a refresh
    void refresh(bool force = false);

    bool isFriend(int userId);
    bool isBlocked(int userId);
    bool isLoaded();

    std::unordered_set<int> getFriends();

private:
    friend class SingletonNodeBase;
    asp::AtomicBool m_fetched = false;
    enum LoadStep {
        None,
        Friends,
        Blocked,
    } m_loadStep;
    std::unordered_set<int> m_friends;
    std::unordered_set<int> m_blocked;
    asp::Mutex<> m_mutex;

    FriendListManager();

    void getUserListFinished(cocos2d::CCArray* p0, UserListType p1);
    void getUserListFailed(UserListType p0, GJErrorCode p1);
    void advance();
};

}