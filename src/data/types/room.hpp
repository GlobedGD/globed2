#pragma once
#include <data/bytebuffer.hpp>

class RoomSettings {
public:
    bool inviteOnly;
    bool publicInvites;
    bool collision;
    bool twoPlayerMode;

    uint64_t reserved;
};

class RoomInfo {
public:
    uint32_t id;
    int owner;
    uint32_t token;
    RoomSettings settings;
};

GLOBED_SERIALIZABLE_STRUCT(RoomInfo, (
    id, owner, token, settings
));
