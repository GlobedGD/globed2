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
    // Returns whether the player is connected to a global room or not.
    Result<bool> isInGlobal();

    // Returns the state of the current room, or an error if the player is not in one.
    Result<RoomData> getRoomData();

    // Attempts to join a room with an id and password.
    // Use an empty string for rooms with no password.
    // Does not provide any visual feedback that the room has been joined or not!
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
