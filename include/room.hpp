#pragma once

#include "_internal.hpp"

namespace globed {
    struct RoomData {
        std::string name;
        std::string password;
        uint32_t id;

        std::string ownerName;
        int32_t ownerAccountId;

        uint16_t playerLimit;
        bool isHidden;
        bool openInvites;
    };

    class RoomJoinEvent : public geode::Event {
    public:
        RoomJoinEvent(RoomData roomData)
            : data(roomData) {}

        inline RoomData getRoomData() { return this->data; }

    private:
        RoomData data;
    };

    class RoomUpdateEvent : public geode::Event {
    public:
        RoomUpdateEvent(RoomData roomData)
            : data(roomData) {}

        inline RoomData getRoomData() { return this->data; }

    private:
        RoomData data;
    };

    class RoomLeaveEvent : public geode::Event {
        
    };
} // namespace globed

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
