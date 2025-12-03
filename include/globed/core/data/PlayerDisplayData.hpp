#pragma once
#include "PlayerIconData.hpp"
#include "SpecialUserData.hpp"
#include <string>

namespace globed {

struct PlayerDisplayData {
    int accountId;
    int userId;
    std::string username;
    PlayerIconData icons;
    std::optional<SpecialUserData> specialUserData;

    static PlayerDisplayData getOwn();
};

static inline const PlayerDisplayData DEFAULT_PLAYER_DATA = {
    .accountId = 0,
    .userId = 0,
    .username = "Player",
    .icons = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 3, NO_GLOW, 1, NO_TRAIL, NO_TRAIL }
};


}
