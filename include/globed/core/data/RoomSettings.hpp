#pragma once
#include <stdint.h>

namespace globed {

struct RoomSettings {
    uint8_t serverId = 0;
    uint16_t playerLimit = 0;
    bool fasterReset = false;
    bool hidden = false;
    bool privateInvites = false;
    bool isFollower = false;
    bool levelIntegrity = false;
    bool teams = false;
    bool lockedTeams = false;
    bool manualPinning = false;

    bool collision = false;
    bool twoPlayerMode = false;
    bool deathlink = false;
};

}
