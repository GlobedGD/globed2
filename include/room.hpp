#pragma once

#include "_internal.hpp"

namespace globed {
    struct RoomData {
        std::string name;
        std::string password;
        std::string owner;
        uint32_t id;
    };
}

namespace globed::room {
    // 
    Result<bool> isInGlobal();

    // 
    Result<RoomData> getRoomData();

    // 
    Result<void> joinRoom(uint32_t roomID, std::string password);
} // namespace globed::room

// Implementation

namespace globed::room {
    inline Result<bool> isInGlobal() {
        return _internal::request<bool>(_internal::Type::IsInGlobal);
    }

    inline Result<RoomData> getRoomData() {
        return _internal::request<RoomData>(_internal::Type::RoomData);
    }

    inline Result<void> joinRoom(uint32_t roomID, std::string password) {
        return _internal::request<void>(_internal::Type::AttemptJoinRoom, roomID, password);
    }

} // namespace globed::room
