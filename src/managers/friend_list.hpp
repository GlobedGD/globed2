#pragma once
#include <defs/geode.hpp>

#include <util/singleton.hpp>

// Class for managing friendlist or blocklist in Geometry Dash.
class FriendListManager : public SingletonBase<FriendListManager> {
protected:
    friend class SingletonBase;
    friend class DummyNode;

    class DummyNode : public UserListDelegate {
    public:
        void cleanup();
        void getUserListFinished(cocos2d::CCArray* p0, UserListType p1);
        void getUserListFailed(UserListType p0, GJErrorCode p1);
    };

public:
    // call `load()` if list has not been loaded yet
    void maybeLoad();

    // reset the friend list
    void invalidate();

    bool isFriend(int playerId);
    bool isBlocked(int playerId);

    std::vector<int> getFriendList();

    bool areFriendsLoaded();
    bool areBlockedLoaded();

private:
    // makes an async request to gd servers and fetches the current friendlist
    void load(bool friends);
    bool isFetching();

    void insertPlayers(cocos2d::CCArray* players, bool friends);

    enum class FetchState {
        None, Friends, Blocked
    };

    DummyNode dummyNode;
    std::set<int> listFriends;
    std::set<int> listBlocked;
    FetchState fetchState;

    bool loadedFriends = false, loadedBlocked = false;
};