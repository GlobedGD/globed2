#pragma once
#include <data/bytebuffer.hpp>
#include <data/bitfield.hpp>

struct RoomSettingsFlags : BitfieldBase {
    bool inviteOnly;
    bool publicInvites;
    bool collision;
    bool twoPlayerMode;
};

GLOBED_SERIALIZABLE_BITFIELD(RoomSettingsFlags, (
    inviteOnly, publicInvites, collision, twoPlayerMode
))

struct RoomSettings {
    RoomSettingsFlags flags;
    uint64_t reserved;
};

GLOBED_SERIALIZABLE_STRUCT(RoomSettings, (
    flags, reserved
))

struct RoomInfo {
    uint32_t id;
    int owner;
    uint32_t token;
    RoomSettings settings;
};

GLOBED_SERIALIZABLE_STRUCT(RoomInfo, (
    id, owner, token, settings
));
