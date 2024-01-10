#pragma once
#include <defs.hpp>

#include <util/sync.hpp>

class RoomManager : GLOBED_SINGLETON(RoomManager) {
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