#pragma once
#include "PlayerAccountData.hpp"
#include <globed/core/SessionId.hpp>

namespace globed {

struct RoomPlayer {
    PlayerAccountData accountData;
    int16_t cube;
    uint16_t color1;
    uint16_t color2;
    SessionId session;
};

}
