#pragma once
#include <data/types/gd.hpp>
#include <data/bytebuffer.hpp>
#include <data/bitfield.hpp>

struct RoomSettingsFlags : BitfieldBase {
    bool isHidden;
    bool publicInvites;
    bool collision;
    bool twoPlayerMode;

    // we need the struct to be 2 bytes
    bool _pad1, _pad2, _pad3, _pad4, _pad5;
};

static_assert((sizeof(RoomSettingsFlags) + 7) / 8 == 2);

GLOBED_SERIALIZABLE_BITFIELD(RoomSettingsFlags, (
    isHidden, publicInvites, collision, twoPlayerMode
))

struct RoomSettings {
    RoomSettingsFlags flags;
    uint32_t playerLimit;
};

GLOBED_SERIALIZABLE_STRUCT(RoomSettings, (
    flags, playerLimit
))

struct RoomInfo {
    uint32_t id;
    PlayerPreviewAccountData owner;
    std::string name;
    std::string password;
    RoomSettings settings;
};

GLOBED_SERIALIZABLE_STRUCT(RoomInfo, (
    id, owner, name, password, settings
));

struct RoomListingInfo {
    uint32_t id;
    PlayerPreviewAccountData owner;
    std::string name;
    bool hasPassword;
    RoomSettings settings;
};

GLOBED_SERIALIZABLE_STRUCT(RoomListingInfo, (
    id, owner, name, hasPassword, settings
));
