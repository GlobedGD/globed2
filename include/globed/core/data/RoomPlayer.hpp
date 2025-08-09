#pragma once
#include "PlayerAccountData.hpp"
#include "PlayerIconData.hpp"
#include <globed/core/SessionId.hpp>

namespace cue { class PlayerIcon; }

namespace globed {

struct RoomPlayer {
    PlayerAccountData accountData;
    int16_t cube = 0;
    uint16_t color1 = 0;
    uint16_t color2 = 0;
    uint16_t glowColor = NO_GLOW;
    SessionId session = SessionId{};

    cue::PlayerIcon* createIcon() const;
    static RoomPlayer createMyself();
};

}
