#pragma once
#include "PlayerAccountData.hpp"
#include "SpecialUserData.hpp"
#include <globed/core/data/ColorId.hpp>
#include <globed/core/SessionId.hpp>

#include <cue/PlayerIcon.hpp>

namespace globed {

struct MinimalRoomPlayer {
    PlayerAccountData accountData;
    int16_t cube = 0;
    ColorId<uint16_t> color1;
    ColorId<uint16_t> color2;
    ColorId<uint16_t> glowColor;

    cue::PlayerIcon* createIcon() const;
    cue::Icons toIcons() const;
    static MinimalRoomPlayer createMyself();
};

struct RoomPlayer : MinimalRoomPlayer {
    SessionId session = SessionId{};
    uint16_t teamId;
    std::optional<SpecialUserData> specialUserData;

    static RoomPlayer createMyself();
};

}
