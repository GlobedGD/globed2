#pragma once
#include "RoomPlayer.hpp"
#include "RoomSettings.hpp"

namespace globed {

struct RoomListingInfo {
    uint32_t roomId;
    std::string roomName;
    RoomPlayer roomOwner;
    int32_t originalOwnerId;
    uint32_t playerCount = 0;
    bool hasPassword = false;
    RoomSettings settings;
};

}
