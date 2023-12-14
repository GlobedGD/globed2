#pragma once
#include <defs.hpp>

#include <util/sync.hpp>

class RoomManager : GLOBED_SINGLETON(RoomManager) {
public:
    util::sync::AtomicU32 roomId = 0;

    inline bool isInGlobal() { return roomId == 0; }
    inline bool isInRoom() { return !isInGlobal(); }

    inline void setRoom(uint32_t roomId) {
        this->roomId = roomId;
    }

    inline void leaveRoom() {
        roomId = 0;
    }
};