#pragma once
#include <asp/sync.hpp>

#include <data/types/room.hpp>

#include <util/singleton.hpp>

class RoomManager : public SingletonBase<RoomManager> {
    friend class SingletonBase;
    RoomManager();

public:
    RoomInfo& getInfo();
    uint32_t getId();

    // Returns 'true' if in a room and if the user is the owner of the room.
    bool isOwner();

    bool isInGlobal();
    bool isInRoom();

    void setInfo(const RoomInfo& info);
    void setGlobal();

private:
    RoomInfo roomInfo;
};