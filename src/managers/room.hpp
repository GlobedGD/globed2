#pragma once
#include <defs/util.hpp>

#include <util/sync.hpp>

class RoomManager : public SingletonBase<RoomManager> {
public:
    util::sync::AtomicU32 roomId = 0;

    bool isInGlobal() {
        return roomId == 0;
    }

    bool isInRoom() {
        return !isInGlobal();
    }

    void setRoom(uint32_t roomId) {
        this->roomId = roomId;
    }

    void leaveRoom() {
        roomId = 0;
    }
};