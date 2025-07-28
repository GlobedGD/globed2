/// This file contains all the messages that can be listened to with `NetworkManager`.

#pragma once
#include "PlayerState.hpp"
#include "PlayerDisplayData.hpp"
#include "RoomPlayer.hpp"
#include "RoomSettings.hpp"
#include "Event.hpp"

namespace globed::msg {

struct RoomStateMessage {
    uint32_t roomId;
    int32_t roomOwner;
    std::string roomName;
    std::vector<RoomPlayer> players;
    RoomSettings settings;
};

struct WarpPlayerMessage {
    uint64_t sessionId;
};

struct LevelDataMessage {
    std::vector<PlayerState> players;
    std::vector<int> culled;
    std::vector<PlayerDisplayData> displayDatas;
    std::vector<Event> events;
};

}
