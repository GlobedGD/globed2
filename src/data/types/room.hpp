#pragma once
#include <data/bytebuffer.hpp>
#include <data/bitfield.hpp>

struct RoomSettingsFlags : BitfieldBase {
    bool inviteOnly;
    bool publicInvites;
    bool collision;
    bool twoPlayerMode;

    // we need the struct to be 2 bytes
    bool _pad1, _pad2, _pad3, _pad4, _pad5;
};

static_assert((sizeof(RoomSettingsFlags) + 7) / 8 == 2);

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

struct RoomListingInfo {
    uint32_t id;
    int owner;
    RoomSettings settings;
};

GLOBED_SERIALIZABLE_STRUCT(RoomListingInfo, (
    id, owner, settings
));
